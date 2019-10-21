#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include <shell.h>
#include <command.h>

using Npshell::Shell;

Shell::Shell() {}

void Shell::run() {
	while (true) {
		show_prompt();
		const std::string cmd = read_command();
		if (!parse_command(cmd))
			break;
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

bool Shell::parse_command(const std::string cmd) {
	Command::Cmdlist cmds;
	cmds.push_back(std::string());
	
	for (auto it = cmd.begin(); it < cmd.end(); ++it) {
		auto &now = cmds.back();
		switch (*it) {
			case ' ': // split command arguments
				cmds.push_back(std::string());
				break;
			default:
				now += *it;
		}
	}

	Command::execute(cmds);
	return true;
}