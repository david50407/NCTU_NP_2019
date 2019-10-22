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

using Npshell::Process;
using Npshell::ProcessList;
using Npshell::ProcessManager;

pid_t ProcessManager::fg_pgid = -1;

ProcessManager::ProcessManager() :
	__process_groups() {
	auto ctrl_c_handler = [=] (const int signal) {
		if (fg_pgid != -1) {
			::kill(-fg_pgid, SIGTERM);
			fg_pgid = -1;
		}
	};

	SignalHandler::subscribe(SIGINT, ctrl_c_handler);
	SignalHandler::subscribe(SIGQUIT, ctrl_c_handler);
	SignalHandler::subscribe(SIGCHLD, [=] (const int signal) {
		::waitpid(-1, nullptr, 0);
	});
}

ProcessManager::~ProcessManager() {
}

void ProcessManager::execute_commands(const Command::Chain &chain) {
	ProcessList pl;
	pid_t pgid = -1;

	int lastPipeInfd = -1;
	for (auto it = chain.begin(); it != chain.end(); ++it) {
		auto &command = *it;
		int fd[2];
		if (it + 1 == chain.end()) {
			fd[PipeOut] = -1;
		} else {
			if (!Util::pipe(fd)) {
				std::cerr << "** Connot create pipe **" << ::strerror(errno) << std::endl;
				goto wait_to_kill;
			}
		}
		pl.emplace_back(Process({command, {lastPipeInfd, fd[PipeOut]}, -1}));
		lastPipeInfd = fd[PipeIn];

		auto &process = pl.back();

		int forksleep = 2;

		while ((process.pid = ::fork()) < 0 && errno == EAGAIN) {
			std::cerr << "fork: retry: " << ::strerror(errno) << std::endl;
			waitchld();
			sleep(forksleep);
			forksleep <<= 1;
		}

		if (process.pid == -1) {
			std::cerr << "fork: failed" << std::endl;
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

	if (pgid != -1) {
		fg_pgid = pgid;
		__process_groups[pgid] = pl;
		wait_proc(pgid);
	}
}

void ProcessManager::wait_proc(const pid_t pgid) {
	while (true) {
		siginfo_t sinfo;
		pid_t pid = waitid(P_PGID, pgid, &sinfo, 0 | WEXITED);

		if (pid == -1) {
			if (errno != ECHILD)
				std::cerr << "** Waitpid Error " << errno << " **" << std::endl;
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