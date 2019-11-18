#include <cstdio>
#include <shell.h>

#include <signal_handler.hxx>
#include <shell_exited.hxx>

using Npshell::Shell;
using Npshell::SignalHandler;
using Npshell::shell_exited;

int main(int argc, char **argv, char **envp) {
	Shell sh;
	SignalHandler::init();

	try {
		sh.run();
	} catch (shell_exited &_e) {
		::exit(0);
	}

	return 0;
}