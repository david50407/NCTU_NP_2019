#if !defined(__SHELL_EXITED_HXX__)
#define __SHELL_EXITED_HXX__

#include <exception>

namespace Npshell {
	class shell_exited : public std::exception {
		public:
			virtual const char* what() const throw () {
				return "Shell terminated.";
			}
	};
}; // namespace Npshell

#endif // !defined(__SHELL_EXITED_HXX__)