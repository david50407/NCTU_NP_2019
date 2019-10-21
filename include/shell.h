#if !defined(__SHELL_H__)
#define __SHELL_H__

#include <string>

#include <process_manager.h>

namespace Npshell {
	class Shell {
		public:
			ProcessManager pm;

		public:
			Shell();
			void run();

		private:
			void initialize_env();
			void show_prompt();
			std::string read_command();
			bool builtin_command(const Command::Chain &);
			bool builtin_command_exit(const Command::Chain &);
			bool builtin_command_setenv(const Command::Chain &);
			bool builtin_command_printenv(const Command::Chain &);
	};
}; // namespace Npshell

#endif // !defined(__SHELL_H__)