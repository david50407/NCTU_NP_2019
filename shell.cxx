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
#include <logger.hxx>

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
		const auto cmd_chains = Command::parse_commands(read_command());
		if (cmd_chains.size() == 0) {
			continue;
		}
		
		for (auto &cmds : cmd_chains) {
			pm.decount_requested_pipe();
			if (!builtin_command(cmds)) {
				pm.execute_commands(cmds);
			}
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
		|| builtin_command_$__error(chain)
		|| builtin_command_exit(chain) 
		|| builtin_command_setenv(chain)
		|| builtin_command_printenv(chain)
		;
}

bool Shell::builtin_command_$__error(const Command::Chain &chain) {
	if (chain.front().get_args()[0] == "$__error") {
		DBG("error: " << chain.front().get_args()[1]);
		return true;
	}

	return false;
}

bool Shell::builtin_command_exit(const Command::Chain &chain) {
	if (chain.front().get_args()[0] == "exit") {
		pm.killall();
		::exit(0);
	}

	return false;
}

bool Shell::builtin_command_setenv(const Command::Chain &chain) {
	if (chain.front().get_args()[0] != "setenv")
		return false;

	const auto &args = chain.front().get_args();
	if (args.size() != 3) {
		DBG("Usage: ​setenv [variable name] [value to assign]");
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
		DBG("Usage: printenv [variable name]");
		return true;
	}

	auto name = std::string(args[1]);
	char *env;
	if ((env = ::getenv(name.c_str())) != NULL) {
		std::cout << env << std::endl;
	}

	return true;
}