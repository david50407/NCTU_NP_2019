#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include <shell.h>
#include <command.h>
#include <signal_handler.hxx>

using Npshell::Shell;
using Npshell::Command;
using Npshell::SignalHandler;

Shell::Shell() : pm() {
	auto quit_handler = [=] (const int signal) {
		std::cin.clear();
		std::cout << std::endl;
		show_prompt();
		std::cout.flush();
	};

	SignalHandler::subscribe(SIGINT, quit_handler);
	SignalHandler::subscribe(SIGQUIT, quit_handler);
}

void Shell::run() {
	while (true) {
		show_prompt();
		const auto cmds = Command::parse_commands(read_command());
		if (cmds.size() == 0) {
			continue;
		} else if (cmds.front().get_args()[0] == "$__error") {
			std::cout << "\033[1;31m" << cmds.front().get_args()[1] << "\033[m" << std::endl;
			continue;
		} else if (cmds.front().get_args()[0] == "exit") {
			::exit(0);
		}
			
		pm.execute_commands(cmds);
	}
}

void Shell::show_prompt() {
	std::cout << "% ";
}

std::string Shell::read_command() {
	std::string cmd = "";
	std::getline(std::cin, cmd);
	if (std::cin.eof()) {
		return "exit";
	}

	return cmd;
}