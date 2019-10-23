#if !defined(__UTIL_H__)
#define __UTIL_H__

#include <list>
#include <vector>
#include <string>

namespace Npshell {
	class Util {
		public:
			static int execvp(const std::string, const std::vector<std::string>);
			static bool setpgid(pid_t, pid_t);
			static bool pipe(int [2]);
			static void multiplexer(std::list<int>);
	};
}; // namespace Npshell

#endif // !defined(__UTIL_H__)