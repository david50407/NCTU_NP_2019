#if !defined(__FDPIPE_HXX__)
#define __FDPIPE_HXX__

#include <exception>

#include <util.h>

namespace Npshell {
	struct fdpipe {
		public:
			class cannot_create_pipe : public std::exception {
				public:
					const char* what() const throw () override { return "Cannot create pipe"; }
			}; // class cannot_create_pipe

		public:
			fdpipe() {
				if (int fd[2]; Util::pipe(fd)) {
					input_ = fd[0];
					output_ = fd[1];
				} else {
					throw cannot_create_pipe();
				}
			}
			fdpipe(const fdpipe &other)
				: input_(other.input_), output_(other.output_) {}
			fdpipe(const int input, const int output)
				: input_(input), output_(output) {}

			const int input() const { return input_; }
			const int output() const { return output_; }

		private:
			int input_ = -1;
			int output_ = -1;
	}; // struct fdpipe
}; // namespace Npshell

#endif // !defined(__FDPIPE_HXX__)