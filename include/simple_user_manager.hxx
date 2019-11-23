#if !defined(__SIMPLE_USER_MANAGER_HXX__)
#define __SIMPLE_USER_MANAGER_HXX__

#include <vector>
#include <optional>
#include <functional>
#include <utility>
#include <sstream>
#include <type_traits>
#include <algorithm>
#include <map>

#include <util.h>
#include <logger.hxx>
#include <user_manager.h>

namespace Npshell {
	class SimpleUserManager : public UserManager {
		public:
			SimpleUserManager() : __users(MAX_USERS, std::nullopt) {}
			
		public:
			int insert(UserInfo info) override {
				auto it = ++(__users.begin()); // start from 1
				while (it->has_value()) { ++it; }

				if (it == __users.end()) {
					DBG("Out of user limits.");
					return -1;
				}

				*it = std::make_optional(info);

				auto idx = it - __users.begin();
				info.shell->bind_to_user_manager(std::make_shared<Binder>(idx, this));

				greeting(idx);

				return idx;
			}
			OptionalUserInfo get(int idx) override {
				return idx >= MAX_USERS
					? std::nullopt
					: __users[idx]
				;
			}
			bool remove(int idx) override {
				if (idx >= MAX_USERS) { return false; }

				auto it = __users.begin() + idx;
				if (!it->has_value()) { return false; }

				auto username = (*it)->name;
				*it = std::nullopt;

				goodbye(username);
				
				// Cleanup pipes
				for (auto it = pipes__.begin(); it != pipes__.end(); ) {
					if (it->first.first == idx || it->first.second == idx) {
						::close(it->second.input());
						::close(it->second.output());
						it = pipes__.erase(it);
					} else {
						++it;
					}
				}

				return true;
			}
			std::list<std::pair<int, UserInfo>> list() const override {
				std::list<std::pair<int, UserInfo>> list;

				for (auto it = ++(__users.begin()); it != __users.end(); ++it) {
					if (*it) {
						list.emplace_back(
							std::make_pair(
								it - __users.begin(),
								**it
							)
						);
					}
				}

				return list;
			}

			bool tell(const int idx, const std::string msg) {
				if (auto user = get(idx); user) {
					user->shell->output() << msg << std::flush;
					return true;
				}

				return false;
			}

			void broadcast(const std::string msg) {
				for (auto [idx, user] : list()) {
					user.shell->output() << msg << std::flush;
				}
			}

			bool rename(const int idx, const std::string name) {
				auto user_ref = get_ref(idx);
				
				if (user_ref == nullptr) {
					DBG("Shell not managed");
					return true;
				}
				
				const auto users = list();
				if (const auto it = std::find_if(
						users.begin(), users.end(),
						[name] (auto &pair) { return pair.second.name == name; }
					); it != users.end()) {
					user_ref->shell->error()
						<< "*** User '" << name << "' already exists. ***" << std::endl;

					return false;
				}

				user_ref->name = name;

				std::stringstream ss;
				ss << "*** User from " << user_ref->address
					<< ":" << user_ref->port << " is named '" << name << "'. ***"
					<< std::endl;
				broadcast(ss.str());

				return true;
			}
			
			virtual fdpipe createPipe(const int from, const int to) override {
				auto ident = std::make_pair(from, to);

				if (auto it = pipes__.find(ident); it != pipes__.end()) {
					throw already_piped();
				}

				try {
					pipes__.emplace(ident, fdpipe{});
				} catch (fdpipe::cannot_create_pipe ex) {
					return fdpipe{-1, -1};
				}

				return pipes__[ident];
			}
			virtual fdpipe removePipe(const int from, const int to) override {
				if (auto it = pipes__.find(std::make_pair(from, to));
					it == pipes__.end()) {
					throw pipe_not_exists();
				} else {
					auto pipe = it->second;
					pipes__.erase(it);

					::close(pipe.output());
					return pipe;
				}
			}

		protected:
			UserInfo *get_ref(int idx) {
				if (idx >= MAX_USERS) { return nullptr; }
				if (!__users[idx]) { return nullptr; }
				
				return &(*__users[idx]);
			}

		private:
			std::vector<OptionalUserInfo> __users;
			std::map<std::pair<int, int>, fdpipe> pipes__;

		private:
			inline void greeting(int idx) {
				if (auto user = get(idx); user) {
					tell(idx, GREETING_MESSAGE);

					std::stringstream ss;
					ss << "*** User '" << user->name << "' entered from "
						<< user->address << ":" << user->port << ". ***" << std::endl;

					broadcast(ss.str());
				}
			}

			inline void goodbye(std::string username) {
				std::stringstream ss;
				ss << "*** User '" << username << "' left. ***" << std::endl;

				broadcast(ss.str());
			}
	}; // class SimpleUserManager
}; // namespace Npshell

#endif // !defined(__SIMPLE_USER_MANAGER_HXX__)