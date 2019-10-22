#if !defined(__PROCESS_MANAGER_H__)
#define __PROCESS_MANAGER_H__

#include <map>
#include <list>

#include <command.h>

#define PipeIn 0
#define PipeOut 1

namespace Npshell {
	struct Process {
		Command cmd;
		int fd[2];
		pid_t pid;
	};

	using ProcessList = std::list<Process>;
	class ProcessManager {
		public:
			ProcessManager();
			~ProcessManager();
			void execute_commands(const Command::Chain &);
			void killall();

		private:
			void wait_proc(const pid_t);
			void waitchld(bool = false);

		private:
			std::map<pid_t, ProcessList> __process_groups;
			static pid_t fg_pgid;
	};
}; // namespace Npshell

#endif // !defined(__PROCESS_MANAGER_H__)