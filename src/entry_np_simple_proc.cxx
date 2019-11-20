#include <cstdio>
#include <unordered_map>
#include <unistd.h>

#include <ext/ifdstream>
#include <ext/ofdstream>
#include <socket_server.h>
#include <shell.h>
#include <shell_exited.hxx>
#include <signal_handler.hxx>
#include <simple_user_manager.hxx>

using Npshell::SocketServer;
using Npshell::Shell;
using Npshell::SignalHandler;
using Npshell::shell_exited;
using Npshell::SimpleUserManager;
using Npshell::UserInfo;

int main(int argc, char **argv, char **envp) {
	short port = 5566;

	if (argc > 1) {
		port = (short)::atoi(argv[1]);
	}

	SignalHandler::init();
	SignalHandler::subscribe(SIGINT, [] (const int _signal) {
		::exit(0);
	});

	SimpleUserManager um;
	std::unordered_map<int, int> fd_userid_mapping;

	SocketServer server(port, [&um, &fd_userid_mapping] (int fd) {
		if (auto it = fd_userid_mapping.find(fd); it == fd_userid_mapping.end()) {
			UserInfo info { "", 0, std::make_shared<Shell>(
				std::make_shared<ext::ifdstream>(fd),
				std::make_shared<ext::ofdstream>(fd)
			) };
			fd_userid_mapping[fd] = um.insert(info);
		}

		auto user = *um.get(fd_userid_mapping[fd]);

		try {
			user.shell->yield();
		} catch (shell_exited &_e) {
			um.remove(fd_userid_mapping[fd]);
			fd_userid_mapping.erase(fd);
			return true;
		}

		return false;
	});

	server.set_nonblocking(true);
	server.start();

	return 0;
}