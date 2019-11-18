#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <csignal>
#include <iostream>
#include <string.h>

#include <signal_handler.hxx>
#include <shell.h>
#include <process_manager.h>
#include <util.h>
#include <logger.hxx>

using Npshell::Process;
using Npshell::ProcessList;
using Npshell::ProcessManager;
using Npshell::RequestPipe;
using Npshell::Shell;

pid_t ProcessManager::fg_pgid = -1;

ProcessManager::ProcessManager(Shell *shell) :
	__process_groups(), __request_pipes(), __shell(*shell) {
	SignalHandler::subscribe(SIGCHLD, [=] (const int signal) {
		::waitpid(-1, nullptr, 0);
	});
}

ProcessManager::~ProcessManager() {
}

void ProcessManager::execute_commands(const Command::Chain &chain) {
	ProcessList pl;
	int lastPipeInfd = -1;

	for (auto it = chain.begin(); it != chain.end(); ++it) {
		decount_requested_pipe();

		lastPipeInfd = process_requested_pipe(lastPipeInfd);

		auto &command = *it;
		int fd[2];
		if (it + 1 == chain.end() && command.pipe_to() == -1) {
			fd[PipeOut] = -1;
		} else {
			auto pipeNumber = command.pipe_to() == -1 ? 1 : command.pipe_to();
			if (auto pipe = find_requested_pipe(pipeNumber); pipe) {
				fd[PipeIn] = pipe->get().fd[PipeIn];
				fd[PipeOut] = pipe->get().fd[PipeOut];
			} else if (!Util::pipe(fd)) {
				DBG("pipe: failed: " << ::strerror(errno));
				goto wait_to_kill;
			}
		}
		pl.emplace_back(Process({command, {lastPipeInfd, fd[PipeOut]}, -1}));
		lastPipeInfd = fd[PipeIn];

		auto &process = pl.back();

		int forksleep = 1;
		while ((process.pid = ::fork()) < 0 && errno == EAGAIN) {
			DBG("fork: retry: " << ::strerror(errno));
			waitchld();
			sleep(forksleep);
			forksleep <<= 1;
			if (forksleep > 8) { forksleep = 8; }
		}

		if (process.pid == -1) {
			DBG("fork: failed: " << ::strerror(errno));
			goto wait_to_kill;
		}

		if (process.pid > 0) { // parent
			Util::setpgid(process.pid, pl.front().pid);

			if (process.fd[PipeIn] != -1)
				::close(process.fd[PipeIn]);
			if (process.fd[PipeOut] != -1 && process.cmd.pipe_to() == -1)
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
		} else if (int fd = __shell.getInputFd(); fd != PipeIn) {
			::dup2(fd, PipeIn);
		}

		if (process.fd[PipeOut] != -1) {
			::dup2(process.fd[PipeOut], PipeOut);
			if (process.cmd.redirect_stderr()) {
				::dup2(process.fd[PipeOut], PipeErr);
			} else if (int fd = __shell.getErrorFd(); fd != PipeErr) {
				::dup2(fd, PipeErr);
			}
			::close(process.fd[PipeOut]);
		} else if (int fd = __shell.getOutputFd(); fd != PipeOut) {
			::dup2(fd, PipeOut);
			::dup2(fd, PipeErr);
		}

		Util::execvp(process.cmd.get_args()[0], process.cmd.get_args());
	}

wait_to_kill: ;

	if (pl.empty()) {
		return;
	}

	auto pgid = pl.front().pid;
	fg_pgid = pgid;
	__process_groups[pgid] = pl;

	if (pl.back().cmd.pipe_to() == -1) {
		wait_proc(pgid);
		return;
	}
	
	// Pipe to n-th command
	auto pipe_to_n = pl.back().cmd.pipe_to();
	if (auto pipe = find_requested_pipe(pipe_to_n); !pipe) {
		__request_pipes.emplace_back(RequestPipe{pgid, pipe_to_n, {lastPipeInfd, pl.back().fd[PipeOut]}});
	}
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

		if ((--(it->request_number)) < 0) {
			it = __request_pipes.erase(it);
		} else {
			++it;
		}
	}
}

std::optional<std::reference_wrapper<RequestPipe>> ProcessManager::find_requested_pipe(int number) {
	auto requestPipe = std::find_if(
		__request_pipes.begin(), __request_pipes.end(),
		[=] (RequestPipe &p) { return p.request_number == number; }
	);
	return requestPipe == __request_pipes.end()
		? std::nullopt
		: std::optional<std::reference_wrapper<RequestPipe>>{*requestPipe}
		;
}

int ProcessManager::process_requested_pipe(int defaultFd) {
	auto pipe = find_requested_pipe(0);
	if (!pipe) {
		return defaultFd;
	}
	
	::close(pipe->get().fd[PipeOut]);
	return pipe->get().fd[PipeIn];
}