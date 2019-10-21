#include <cstdio>
#include <shell.h>

using Npshell::Shell;

int main(int argc, char **argv, char **envp) {
	Shell sh;

	sh.run();

	return 0;
}