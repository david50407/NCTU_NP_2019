#include <unistd.h>
#include <pwd.h>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include <shell.h>
#include <command.h>
#include <signal_handler.hxx>
#include <logger.hxx>
#include <shell_exited.hxx>

using Npshell::Shell;
using Npshell::Command;
using Npshell::SignalHandler;
using Npshell::shell_exited;

Shell::Shell()
	: pm(this), envs({{ "PATH", "bin:." }})
	, input_stream_(nullptr), output_stream_(nullptr), error_stream_(nullptr) {
	register_signal();
}

Shell::Shell(int inputFd, int outputFd)
	: pm(this), envs({{ "PATH", "bin:." }})
	, input_stream_(new ext::ifdstream(inputFd))
	, output_stream_(new ext::ofdstream(outputFd))
	, error_stream_(nullptr) {
	error_stream_ = output_stream_;
}

Shell::Shell(int inputFd, int outputFd, int errorFd)
	: pm(this), envs({{ "PATH", "bin:." }})
	, input_stream_(new ext::ifdstream(inputFd))
	, output_stream_(new ext::ofdstream(outputFd))
	, error_stream_(new ext::ofdstream(errorFd))
	{}

void Shell::register_signal() {
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
		const auto cmd_chains = Command::parse_commands(read_command());
		if (cmd_chains.size() == 0) {
			continue;
		}
		
		for (auto &cmds : cmd_chains) {
			if (builtin_command(cmds)) {
				pm.decount_requested_pipe(); // Decount for built-in commands
			} else {
				pm.execute_commands(cmds);
			}
		}
	}
}

void Shell::show_prompt() {
	output() << "% " << std::flush;
}

std::string Shell::read_command() {
	std::string cmd = "";
	std::getline(input(), cmd);
	if (input().eof()) {
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
		throw shell_exited();
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
	envs.set(name, value);

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
	if (auto env = envs.get(name); env) {
		output() << *env << std::endl;
	}

	return true;
}