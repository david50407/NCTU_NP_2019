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

	::execvp(program.c_str(), c_args.data()); // won't return
	std::cerr << "\033[1;31m" << "Cannot exec: " << program.c_str() << "\033[m" << std::endl;
	::exit(errno);

	return 0;
}

bool Util::setpgid(pid_t pid, pid_t pgid) {
	return ::setpgid(pid, pgid) == 0;
}

bool Util::pipe(int fd[2]) {
	return ::pipe(fd) == 0;
}