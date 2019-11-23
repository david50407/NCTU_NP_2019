#if !defined(__COMMAND_H__)
#define __COMMAND_H__

#include <list>
#include <vector>
#include <string>
#include <initializer_list>

namespace Npshell {
	class Command {
		public:
			using Cmdlist = std::vector<std::string>;
			using Chain = std::vector<class Command>;
		
		private:
			std::vector<std::string> args;
			std::string redirect_out = "";
			int pipe_to_n = -1;
			bool pipe_stderr = false;
			int pipe_to_uid = -1;
			int pipe_from_uid = -1;

		public:
			Command();
			Command(const std::initializer_list<std::string>);
			~Command();
			std::vector<std::string> get_args() const { return args; }
			std::string get_redirect_out() { return redirect_out; }
			int pipe_to() const { return pipe_to_n; }
			bool redirect_stderr() const { return pipe_stderr; }
			int to_uid() const { return pipe_to_uid; }
			int from_uid() const { return pipe_from_uid; }

			static std::list<Chain> parse_commands(const std::string &, const bool);
	};
}; // namespace Npshell

#endif // !defined(__COMMAND_H__)