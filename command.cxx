#include <string>

#include <command.h>
#include <util.h>

using Npshell::Command;

bool Command::execute(const Command::Cmdlist cmds) {
	return cmd_exit(cmds) || cmd_execute(cmds);
}

bool Command::cmd_exit(const Command::Cmdlist &cmds) {
	if ("exit" == cmds[0])
		exit(0);
	return false;
}

bool Command::cmd_execute(const Command::Cmdlist &cmds) {
	const int status = Util::execvp(cmds[0], cmds);
	return status == 0;
}