#include <cstring>
#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include <util.h>

using Npshell::Util;

int Util::execvp(const std::string program, const std::vector<std::string> args) {
	std::vector<char *> c_args;
	std::transform(args.begin(), args.end(), std::back_inserter(c_args), [](const std::string &str) -> char * {
			const auto str_size = str.size();
			char *c_str = new char[str_size + 1];
			std::strncpy(c_str, str.c_str(), str_size);
			c_str[str_size] = '\0';
			return c_str;
			});
	c_args.push_back(NULL);

	pid_t child_pid;
	int status;
	if ((child_pid = fork()) < 0) {
		std::cerr << "** Cannot fork **";
	} else if (child_pid == 0) { // child process
		::execvp(program.c_str(), c_args.data()); // won't return
		std::cerr << "** Cannot exec **";
		::exit(errno);
	} else { // parent
		while (::waitpid(child_pid, &status, WNOHANG) != child_pid);
	}

	for (auto i = c_args.size() - 2; i != 0; --i)
		delete[] c_args[i];
	return 0;
}