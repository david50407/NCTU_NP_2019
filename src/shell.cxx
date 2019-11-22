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

Shell::Shell(std::shared_ptr<std::istream> input_ptr, std::shared_ptr<std::ostream> output_ptr)
	: pm(this), envs({{ "PATH", "bin:." }})
	, input_stream_(std::forward<decltype(input_ptr)>(input_ptr))
	, output_stream_(std::forward<decltype(output_ptr)>(output_ptr))
	, error_stream_(nullptr) {
	error_stream_ = output_stream_;
}

Shell::Shell(std::shared_ptr<std::istream> input_ptr, std::shared_ptr<std::ostream> output_ptr, std::shared_ptr<std::ostream> error_ptr)
	: pm(this), envs({{ "PATH", "bin:." }})
	, input_stream_(std::forward<decltype(input_ptr)>(input_ptr))
	, output_stream_(std::forward<decltype(output_ptr)>(output_ptr))
	, error_stream_(std::forward<decltype(error_ptr)>(error_ptr))
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
		yield();
	}
}

void Shell::yield() {
	if (welcome_) {
		const auto cmd_chains = Command::parse_commands(read_command());
		if (cmd_chains.size() == 0) {
			return;
		}
		
		for (auto &cmds : cmd_chains) {
			if (builtin_command(cmds)) {
				pm.decount_requested_pipe(); // Decount for built-in commands
			} else {
				pm.execute_commands(cmds);
			}
		}
	}

	welcome_ = true;
	show_prompt();
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
		|| builtin_command_yell(chain)
		|| builtin_command_tell(chain)
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

bool Shell::builtin_command_yell(const Command::Chain &chain) {
	if (binded_user_manager_ == nullptr) { return false; }
	if (chain.front().get_args()[0] != "yell") { return false; }

	const auto &args = chain.front().get_args();
	if (args.size() == 1) {
		DBG("Usage: yell [message]...");
		return true;
	}

	const auto user = binded_user_manager_->get(this);
	if (!user) {
		DBG("Not registered to user manager");
		return false;
	}

	std::stringstream message;
	message << "*** " << user->name << " yelled ***:";
	for (auto it = ++args.begin(); it != args.end(); ++it) {
		message << " " << *it;
	}
	message << std::endl;

	binded_user_manager_->broadcast(message.str());

	return true;
}

bool Shell::builtin_command_tell(const Command::Chain &chain) {
	if (binded_user_manager_ == nullptr) { return false; }
	if (chain.front().get_args()[0] != "tell") { return false; }

	const auto &args = chain.front().get_args();
	if (args.size() < 3) {
		DBG("Usage: tell [id] [message]...");
		return true;
	}

	const auto user = binded_user_manager_->get(this);
	if (!user) {
		DBG("Not registered to user manager");
		return false;
	}

	auto target_idx = ::atoi(args[1].c_str());
	std::stringstream message;
	message << "*** " << user->name << " told you ***:";
	for (auto it = args.begin() + 2; it != args.end(); ++it) {
		message << " " << *it;
	}
	message << std::endl;

	if (!binded_user_manager_->tell(message.str(), { target_idx })) {
		error() 
			<< "*** Error: user #" << target_idx 
			<< " does not exist yet. ***" << std::endl;
	}

	return true;
}