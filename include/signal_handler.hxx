#if !defined(__SIGNAL_HANDLER_HXX__)
#define __SIGNAL_HANDLER_HXX__

#include <csignal>
#include <functional>
#include <map>
#include <list>

namespace Npshell {
	class SignalHandler {
		public:
			using Callback = std::function<void (int)>;

		public:
			static void init() {
				std::signal(SIGTTOU, SIG_IGN);
				std::signal(SIGTTIN, SIG_IGN);
				std::signal(SIGINT, [] (int signal) {
						trigger(signal);
				});
				std::signal(SIGQUIT, [] (int signal) {
						trigger(signal);
				});
				std::signal(SIGCHLD, [] (int signal) {
						trigger(signal);
				});
				std::signal(SIGUSR1, [] (int signal) {
						trigger(signal);
				});
				std::signal(SIGUSR2, [] (int signal) {
						trigger(signal);
				});
			}
			static void subscribe(const int signal, const Callback &callback) {
				__subscribers[signal].push_back(callback);
			}
			static void cleanup(const int signal) {
				__subscribers[signal].clear();
			}
			static void trigger(const int signal) {
				for (auto &callback : __subscribers[signal]) {
					callback(signal);
				}
			}
			static void enter_child_mode() {
				std::signal(SIGTTOU, SIG_DFL);
				std::signal(SIGTTIN, SIG_DFL);
				std::signal(SIGCHLD, SIG_DFL);
				std::signal(SIGINT, SIG_DFL);
				std::signal(SIGQUIT, SIG_DFL);
				std::signal(SIGTSTP, SIG_DFL);
				std::signal(SIGCONT, SIG_DFL);
				std::signal(SIGUSR1, SIG_DFL);
				std::signal(SIGUSR2, SIG_DFL);
			}

		private:
			static std::map<int, std::list<Callback>> __subscribers;
	};
}; // namespace Npshell

#endif // !defined(__SIGNAL_HANDLER_HXX__)