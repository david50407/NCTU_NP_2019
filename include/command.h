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
			std::string redirect_out;

		public:
			~Command();
			std::vector<std::string> get_args() const { return args; }
			std::string get_redirect_out() { return redirect_out; }
			static Chain parse_commands(const std::string &);

		private:
			Command();
			Command(const std::initializer_list<std::string>);
	};
}; // namespace Npshell

#endif // !defined(__COMMAND_H__)