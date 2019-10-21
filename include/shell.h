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
			void show_prompt();
			std::string read_command();
	};
}; // namespace Npshell

#endif // !defined(__SHELL_H__)