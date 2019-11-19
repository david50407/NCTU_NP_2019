#if !defined(__SHELL_H__)
#define __SHELL_H__

#include <string>
#include <iostream>
#include <typeinfo>

#include <ext/ifdstream>
#include <ext/ofdstream>
#include <process_manager.h>
#include <environment.hxx>

namespace Npshell {
	class Shell {
		public:
			ProcessManager pm;
			Environment envs;

		public:
			Shell();
			Shell(std::istream&, std::ostream&);
			Shell(std::istream&, std::ostream&, std::ostream&);
			void run();
			const int getInputFd() {
				return typeid(_input) == typeid(ext::ifdstream)
					? dynamic_cast<ext::ifdstream &>(_input).getFd()
					: STDIN_FILENO
					;
			}
			const int getOutputFd() {
				return typeid(_output) == typeid(ext::ofdstream)
					? dynamic_cast<ext::ofdstream &>(_output).getFd()
					: STDOUT_FILENO
					;
			}
			const int getErrorFd() {
				return typeid(_error) == typeid(ext::ofdstream)
					? dynamic_cast<ext::ofdstream &>(_error).getFd()
					: STDERR_FILENO
					;
			}

		private:
			std::istream& _input;
			std::ostream& _output;
			std::ostream& _error;

		private:
			void show_prompt();
			std::string read_command();
			bool builtin_command(const Command::Chain &);
			bool builtin_command_$__error(const Command::Chain &);
			bool builtin_command_exit(const Command::Chain &);
			bool builtin_command_setenv(const Command::Chain &);
			bool builtin_command_printenv(const Command::Chain &);
	};
}; // namespace Npshell

#endif // !defined(__SHELL_H__)