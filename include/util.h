#if !defined(__UTIL_H__)
#define __UTIL_H__

#include <list>
#include <vector>
#include <string>

#include <environment.hxx>

namespace Npshell {
	class Util {
		public:
			static int execvpe(const std::string, const std::vector<std::string>, const Environment &);
			static bool setpgid(pid_t, pid_t);
			static bool pipe(int [2]);
			static void multiplexer(std::list<int>);
	};
}; // namespace Npshell

#endif // !defined(__UTIL_H__)