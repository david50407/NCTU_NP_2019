#if !defined(__SHELL_H__)
#define __SHELL_H__

#include <string>
#include <iostream>

#include <process_manager.h>

namespace Npshell {
	class Shell {
		public:
			ProcessManager pm;

		public:
			Shell();
			Shell(std::istream&, std::ostream&);
			void run();

		private:
			std::istream& _input;
			std::ostream& _output;

		private:
			void initialize_env();
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