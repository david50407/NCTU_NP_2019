#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <csignal>
#include <iostream>
#include <string.h>

#include <signal_handler.hxx>
#include <process_manager.h>
#include <util.h>
#include <logger.hxx>

using Npshell::Process;
using Npshell::ProcessList;
using Npshell::ProcessManager;
using Npshell::RequestPipe;

pid_t ProcessManager::fg_pgid = -1;

ProcessManager::ProcessManager() :
	__process_groups(), __request_pipes() {
	SignalHandler::subscribe(SIGCHLD, [=] (const int signal) {
		::waitpid(-1, nullptr, 0);
	});
}

ProcessManager::~ProcessManager() {
}

void ProcessManager::execute_commands(const Command::Chain &chain) {
	ProcessList pl;
	int lastPipeInfd = process_requested_pipe(pl);
	pid_t pgid = pl.empty() ? -1 : pl.back().pid;

	for (auto it = chain.begin(); it != chain.end(); ++it) {
		auto &command = *it;
		int fd[2];
		if (it + 1 == chain.end() && command.pipe_to() == -1) {
			fd[PipeOut] = -1;
		} else {
			if (!Util::pipe(fd)) {
				DBG("pipe: failed: " << ::strerror(errno));
				goto wait_to_kill;
			}
		}
		pl.emplace_back(Process({command, {lastPipeInfd, fd[PipeOut]}, -1}));
		lastPipeInfd = fd[PipeIn];

		auto &process = pl.back();

		int forksleep = 2;

		while ((process.pid = ::fork()) < 0 && errno == EAGAIN) {
			DBG("fork: retry: " << ::strerror(errno));
			waitchld();
			sleep(forksleep);
			forksleep <<= 1;
		}

		if (process.pid == -1) {
			DBG("fork: failed: " << ::strerror(errno));
			goto wait_to_kill;
		}

		if (process.pid > 0) { // parent
			if (pgid == -1)
				pgid = process.pid;
			Util::setpgid(process.pid, pgid);

			if (process.fd[PipeIn] != -1)
				::close(process.fd[PipeIn]);
			if (process.fd[PipeOut] != -1)
				::close(process.fd[PipeOut]);
			continue;
		}

		// child
		SignalHandler::enter_child_mode();

		if (process.cmd.get_redirect_out().size() > 0)
			::freopen(process.cmd.get_redirect_out().c_str(), "w+", stdout);
		if (process.fd[PipeIn] != -1) {
			::dup2(process.fd[PipeIn], PipeIn);
			::close(process.fd[PipeIn]);
		}
		if (process.fd[PipeOut] != -1) {
			::dup2(process.fd[PipeOut], PipeOut);
			::close(process.fd[PipeOut]);
		}

		Util::execvp(process.cmd.get_args()[0], process.cmd.get_args());
	}

wait_to_kill: ;

	if (pgid == -1) {
		return;
	}

	fg_pgid = pgid;
	__process_groups[pgid] = pl;

	if (pl.back().cmd.pipe_to() == -1) {
		wait_proc(pgid);
		return;
	}
	
	// Pipe to n-th command
	auto pipe_to_n = pl.back().cmd.pipe_to();
	__request_pipes.emplace_back(RequestPipe{pgid, pipe_to_n, lastPipeInfd});
}

void ProcessManager::wait_proc(const pid_t pgid) {
	while (true) {
		siginfo_t sinfo;
		pid_t pid = waitid(P_PGID, pgid, &sinfo, 0 | WEXITED);

		if (pid == -1) {
			if (errno != ECHILD)
				DBG("waitid: retry: " << ::strerror(errno));
			break;
		}
	}

	fg_pgid = -1;
	__process_groups.erase(pgid);
}

void ProcessManager::waitchld(bool block) {
	int status;
	pid_t pid;

	do {
		pid = ::waitpid(-1, &status, WEXITED | (block ? 0 : WNOHANG));

		if (pid == -1 && errno == ECHILD) {
			break;
		}
	} while (pid > (pid_t) 0);
}

void ProcessManager::killall() {
	for (auto pair : __process_groups) {
		::kill(-pair.first, SIGTERM);
	}
}

void ProcessManager::decount_requested_pipe() {
	for (auto it = __request_pipes.begin(); it != __request_pipes.end(); ) {
		if (__process_groups.find(it->pgid) == __process_groups.end()) {
			// Clean up & step in
			it = __request_pipes.erase(it);
			continue;
		}

		--((it++)->request_number);
	}
}

int ProcessManager::process_requested_pipe(ProcessList &pl) {
	std::list<RequestPipe> requested_pipes = {};
	for (auto it = __request_pipes.begin(); it != __request_pipes.end(); ) {
		if (it->request_number > 0) {
			++it;
			continue;
		}

		requested_pipes.push_back(*it);
		it = __request_pipes.erase(it);
	}

	if (requested_pipes.empty()) {
		return -1;
	}

	int fd[2];
	if (!Util::pipe(fd)) {
		DBG("pipe: failed: " << ::strerror(errno));
	}
	pl.emplace_back(Process({{"multiplexer"}, {-1, fd[PipeOut]}, -1}));

	auto &process = pl.back();
	int forksleep = 2;

	while ((process.pid = ::fork()) < 0 && errno == EAGAIN) {
		DBG("fork: retry: " << ::strerror(errno));
		waitchld();
		sleep(forksleep);
		forksleep <<= 1;
	}

	if (process.pid == -1) {
		DBG("fork: failed: " << ::strerror(errno));
		return -1;
	}

	if (process.pid > 0) { // parent
		Util::setpgid(process.pid, process.pid);
		::close(process.fd[PipeOut]);
		return fd[PipeIn];
	}

	// child
	SignalHandler::enter_child_mode();
	// Cleanup
	__process_groups.clear();
	for (auto &other_request : __request_pipes) {
		::close(other_request.outfd);
	}
	__request_pipes.clear();

	::dup2(process.fd[PipeOut], PipeOut);
	::close(process.fd[PipeOut]);
	::close(fd[PipeIn]);

	std::list<int> fds;
	std::transform(requested_pipes.begin(), requested_pipes.end(), std::back_inserter(fds), [](RequestPipe &req) -> int {
		return req.outfd;
	});

	Util::multiplexer(fds);

	::exit(0);
	return -1;
}