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

	struct RequestPipe {
		pid_t pgid;
		int request_number;
		int outfd;
	};

	using ProcessList = std::list<Process>;
	using RequestPipeList = std::list<RequestPipe>;
	class ProcessManager {
		public:
			ProcessManager();
			~ProcessManager();
			void execute_commands(const Command::Chain &);
			void killall();
			void decount_requested_pipe();

		private:
			void wait_proc(const pid_t);
			void waitchld(bool = false);
			int process_requested_pipe(ProcessList &);

		private:
			std::map<pid_t, ProcessList> __process_groups;
			RequestPipeList __request_pipes;
			static pid_t fg_pgid;
	};
}; // namespace Npshell

#endif // !defined(__PROCESS_MANAGER_H__)