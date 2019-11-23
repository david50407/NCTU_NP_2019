#include <cstdio>
#include <unordered_map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <ext/ifdstream>
#include <ext/ofdstream>
#include <socket_server.h>
#include <shell.h>
#include <shell_exited.hxx>
#include <signal_handler.hxx>
#include <shm_user_manager.hxx>

using Npshell::SocketServer;
using Npshell::Shell;
using Npshell::SignalHandler;
using Npshell::shell_exited;
using Npshell::ShmUserManager;
using Npshell::UserInfo;
using Npshell::ClientInfo;

int main(int argc, char **argv, char **envp) {
	unsigned short port = 5566;

	if (argc > 1) {
		port = (unsigned short)::atoi(argv[1]);
	}

	SignalHandler::init();
	SignalHandler::subscribe(SIGQUIT, [] (const int _signal) {
		ShmUserManager::cleanup_shm();
		::exit(0);
	});
	SignalHandler::subscribe(SIGINT, [] (const int _signal) {
		ShmUserManager::cleanup_shm();
		::exit(0);
	});
	SignalHandler::subscribe(SIGCHLD, [] (const int signal) {
		::waitpid(-1, nullptr, 0);
	});

	ShmUserManager um;

	SocketServer server(port, [&um] (int fd, std::optional<ClientInfo> client_info) {
		pid_t pid = ::fork();
		if (pid == -1) {
			DBG("fork: failed: " << ::strerror(errno));
			return true;
		}

		if (pid > 0) { // Parent
			return true;
		}

		SignalHandler::cleanup(SIGQUIT);
		SignalHandler::cleanup(SIGINT);
		SignalHandler::cleanup(SIGCHLD);
		SignalHandler::subscribe(SIGQUIT, [] (const int _signal) {
			::exit(0);
		});
		SignalHandler::subscribe(SIGINT, [] (const int _signal) {
			::exit(0);
		});

		UserInfo user {
			client_info->address,
			client_info->port,
			std::make_shared<Shell>(
				std::make_shared<ext::ifdstream>(fd),
				std::make_shared<ext::ofdstream>(fd)
			)
		};

		auto uid = um.insert(user);

		try {
			user.shell->run();
		} catch (shell_exited &_e) {}

		um.remove(uid);
		::exit(0);

		return true;
	});

	server.start();

	ShmUserManager::cleanup_shm();
	return 0;
}