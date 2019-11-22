#if !defined(__SHELL_H__)
#define __SHELL_H__

#include <string>
#include <iostream>
#include <typeinfo>
#include <functional>
#include <optional>
#include <utility>
#include <memory>

#include <ext/ifdstream>
#include <ext/ofdstream>
#include <process_manager.h>
#include <environment.hxx>
#include <user_manager.h>

namespace Npshell {
	class Shell {
		public:
			ProcessManager pm;
			Environment envs;

		public:
			Shell();
			Shell(std::shared_ptr<std::istream>, std::shared_ptr<std::ostream>);
			Shell(std::shared_ptr<std::istream>, std::shared_ptr<std::ostream>, std::shared_ptr<std::ostream>);
			void run();
			void yield();
			const int getInputFd() {
				if (auto &stream = input(); typeid(stream) == typeid(ext::ifdstream)) {
					return dynamic_cast<const ext::ifdstream &>(stream).getFd();
				}
				
				return STDIN_FILENO;
			}
			const int getOutputFd() {
				if (auto &stream = output(); typeid(stream) == typeid(ext::ofdstream)) {
					return dynamic_cast<const ext::ofdstream &>(stream).getFd();
				}
				
				return STDOUT_FILENO;
			}
			const int getErrorFd() {
				if (auto &stream = error(); typeid(stream) == typeid(ext::ofdstream)) {
					return dynamic_cast<const ext::ofdstream &>(stream).getFd();
				}
				
				return STDERR_FILENO;
			}
			std::istream &input() {
				return input_stream_ ? *input_stream_ : std::cin;
			}
			std::ostream &output() {
				return output_stream_ ? *output_stream_ : std::cout;
			}
			std::ostream &error() {
				return error_stream_ ? *error_stream_ : (output_stream_ ? *output_stream_ : std::cerr);
			}
			void bind_to_user_manager(std::unique_ptr<UserManager::Binder> binder) {
				binded_user_manager_ = std::move(binder);
			}

		private:
			std::unique_ptr<UserManager::Binder> binded_user_manager_ = nullptr;
			std::shared_ptr<std::istream> input_stream_;
			std::shared_ptr<std::ostream> output_stream_;
			std::shared_ptr<std::ostream> error_stream_;
			bool welcome_ = false;
			static const std::string WHO_HEADER;

		private:
			void register_signal();
			void show_prompt();
			std::string read_command();
			bool builtin_command(const Command::Chain &);
			bool builtin_command_$__error(const Command::Chain &);
			bool builtin_command_exit(const Command::Chain &);
			bool builtin_command_setenv(const Command::Chain &);
			bool builtin_command_printenv(const Command::Chain &);
			bool builtin_command_yell(const Command::Chain &);
			bool builtin_command_tell(const Command::Chain &);
			bool builtin_command_rename(const Command::Chain &);
			bool builtin_command_who(const Command::Chain &);
	};
}; // namespace Npshell

#endif // !defined(__SHELL_H__)