#include <unistd.h>
#include <pwd.h>
#include <cstdio>
#include <cstdlib>
#include <climits>
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
	initialize_env();

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
		
		if (!builtin_command(cmds)) {
			pm.execute_commands(cmds);
		}
	}
}

void Shell::initialize_env() {
	::setenv("PATH", "bin:.", 1);
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

bool Shell::builtin_command(const Command::Chain &chain) {
	return false
		|| builtin_command_exit(chain) 
		|| builtin_command_setenv(chain)
		|| builtin_command_printenv(chain)
		;
}

bool Shell::builtin_command_exit(const Command::Chain &chain) {
	if (chain.front().get_args()[0] == "exit") {
		pm.killall();
		exit(0);
	}

	return false;
}

bool Shell::builtin_command_setenv(const Command::Chain &chain) {
	if (chain.front().get_args()[0] != "setenv")
		return false;

	const auto &args = chain.front().get_args();
	if (args.size() != 3) {
		std::cerr << "Usage: ​setenv [variable name] [value to assign]" << std::endl;
		return true;
	}

	auto name = std::string(args[1]);
	auto value = std::string(args[2]);
	::setenv(name.c_str(), value.c_str(), 1);

	return true;
}

bool Shell::builtin_command_printenv(const Command::Chain &chain) {
	if (chain.front().get_args()[0] != "printenv")
		return false;

	const auto &args = chain.front().get_args();
	if (args.size() != 2) {
		std::cerr << "Usage: printenv [variable name]" << std::endl;
		return true;
	}

	auto name = std::string(args[1]);
	char *env;
	if ((env = ::getenv(name.c_str())) != NULL) {
		std::cout << env << std::endl;
	}

	return true;
}