#include <cstring>
#include <algorithm>
#include <iostream>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include <ext/ifdstream>
#include <util.h>
#include <logger.hxx>
#include <environment.hxx>

using Npshell::Util;
using Npshell::Environment;

int Util::execvpe(const std::string program, const std::vector<std::string> args, const Environment &envs) {
	std::vector<char *> c_args;
	std::transform(args.begin(), args.end(), std::back_inserter(c_args), [](const std::string &str) -> char * {
			const auto str_size = str.size();
			char *c_str = new char[str_size + 1];
			std::strncpy(c_str, str.c_str(), str_size);
			c_str[str_size] = '\0';
			return c_str;
			});
	c_args.push_back(NULL);

	std::vector<char *> c_envp;
	std::transform(envs.map().begin(), envs.map().end(), std::back_inserter(c_envp),
		[](const std::pair<std::string, std::string> env) -> char * {
			const auto [key, val] = env;
			const auto key_size = key.size();
			const auto val_size = val.size();
			char *c_str = new char[key_size + 1 + val_size + 1];
			std::strncpy(c_str, key.c_str(), key_size);
			std::strncpy(c_str + key_size + 1, val.c_str(), val_size);
			c_str[key_size] = '=';
			c_str[key_size + 1 + val_size] = '\0';
			return c_str;
	});
	c_envp.push_back(NULL);

	::setenv("PATH", envs.get("PATH").value_or("").c_str(), 1);
	::execvpe(program.c_str(), c_args.data(), c_envp.data()); // won't return
	std::cerr << "Unknown command: [" << program.c_str() << "]." << std::endl;
	::exit(errno);

	return 0;
}

bool Util::setpgid(pid_t pid, pid_t pgid) {
	return ::setpgid(pid, pgid) == 0;
}

bool Util::pipe(int fd[2]) {
	return ::pipe(fd) == 0;
}

void Util::multiplexer(std::list<int> fds) {
	char buffer[1024 + 1];

	for (auto fd : fds) {
		ext::ifdstream fdin(fd);
		
		while (!fdin.eof()) {
			fdin.read(buffer, sizeof(buffer) - 1);
			if (auto size = fdin.gcount(); size > 0) {
				buffer[size] = '\0';
				std::cout << buffer << std::flush;
			}
		}

		::close(fd);
	}
}