#include <cstdio>
#include <unistd.h>

#include <ext/ifdstream>
#include <ext/ofdstream>
#include <socket_server.h>
#include <shell.h>
#include <shell_exited.hxx>
#include <signal_handler.hxx>

using Npshell::SocketServer;
using Npshell::Shell;
using Npshell::SignalHandler;
using Npshell::shell_exited;
using Npshell::ClientInfo;

int main(int argc, char **argv, char **envp) {
	unsigned short port = 5566;

	if (argc > 1) {
		port = (unsigned short)::atoi(argv[1]);
	}

	SignalHandler::init();
	SignalHandler::subscribe(SIGINT, [] (const int _signal) {
		::exit(0);
	});

	SocketServer server(port, [] (int fd, std::optional<ClientInfo> client_info) {
		Shell sh(
			std::make_shared<ext::ifdstream>(fd),
			std::make_shared<ext::ofdstream>(fd)
		);

		try {
			sh.run();
		} catch (shell_exited &_e) {}

		return true;
	});

	server.start();

	return 0;
}