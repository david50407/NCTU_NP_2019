#include <cstdio>
#include <shell.h>

#include <signal_handler.hxx>

using Npshell::Shell;
using Npshell::SignalHandler;

int main(int argc, char **argv, char **envp) {
	Shell sh;
	SignalHandler::init();
	sh.run();

	return 0;
}