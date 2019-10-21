#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <csignal>
#include <iostream>

#include <signal_handler.hxx>
#include <process_manager.h>
#include <util.h>

using Npshell::Process;
using Npshell::ProcessList;
using Npshell::ProcessManager;

static pid_t fg_pgid;

ProcessManager::ProcessManager() :
	__process_groups() {
}

ProcessManager::~ProcessManager() {
}

void ProcessManager::execute_commands(const Command::Chain &chain) {
	ProcessList pl;

	Process *last = nullptr;
	for (auto &command : chain) {
		pl.emplace_back(Process({command, {-1, -1}, -1}));
		if (last != nullptr) {
			int fd[2];
			if (!Util::pipe(fd)) {
				std::cerr << "** Connot create pipe **" << std::endl;
				return;
			}

			last->fd[PipeOut] = fd[PipeOut];
			pl.back().fd[PipeIn] = fd[PipeIn];
		}
		last = &pl.back();
	}

	pid_t pgid = -1;
	for (auto &process : pl) {
		process.pid = ::fork();
		if (process.pid < 0) { // fail
			std::cerr << "** Cannot fork**" << std::endl;
			return;
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
		if (process.fd[PipeIn] != -1)
			::dup2(process.fd[PipeIn], PipeIn);
		if (process.fd[PipeOut] != -1)
			::dup2(process.fd[PipeOut], PipeOut);

		Util::execvp(process.cmd.get_args()[0], process.cmd.get_args());
	}

	__process_groups[pgid] = pl;
	wait_proc(pgid);
}

void ProcessManager::wait_proc(const pid_t pgid) {
	while (true) {
		siginfo_t sinfo;
		pid_t pid = waitid(P_PGID, pgid, &sinfo, 0 | WUNTRACED);

		if (pid == -1) {
			if (errno != ECHILD)
				std::cerr << "** Waitpid Error " << errno << " **" << std::endl;
			break;
		}
	}
}