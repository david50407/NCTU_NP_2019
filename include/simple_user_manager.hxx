#if !defined(__SIMPLE_USER_MANAGER_HXX__)
#define __SIMPLE_USER_MANAGER_HXX__

#include <vector>
#include <optional>
#include <functional>
#include <utility>
#include <sstream>
#include <type_traits>
#include <algorithm>

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
				info.shell->bind_to_user_manager(std::make_unique<Binder>(idx, this));

				greeting(idx);

				return idx;
			}
			OptionalUserInfo get(int idx) override {
				return idx >= MAX_USERS
					? std::nullopt
					: __users[idx]
				;
			}
			OptionalUserInfo get(const Shell *shell_ptr) override {
				auto it = std::find_if(__users.begin(), __users.end(), [shell_ptr] (auto &info) -> bool {
					return info
						? info->shell.get() == shell_ptr
						: false;
				});

				return it == __users.end()
					? std::nullopt
					: *it;
			}
			bool remove(int idx) override {
				if (idx >= MAX_USERS) { return false; }

				auto it = __users.begin() + idx;
				if (!it->has_value()) { return false; }

				auto username = (*it)->name;
				*it = std::nullopt;

				goodbye(username);
				return true;
			}
			const std::list<std::pair<int, const std::reference_wrapper<const UserInfo>>> list() const override {
				typename std::remove_const<decltype(list())>::type list;

				for (auto it = ++(__users.begin()); it != __users.end(); ++it) {
					if (*it) {
						list.emplace_back(
							std::make_pair<int, std::reference_wrapper<const UserInfo>>(
								it - __users.begin(),
								std::cref(**it)
							)
						);
					}
				}

				return list;
			}

		protected:
			UserInfo *get_ref(int idx) override {
				if (idx >= MAX_USERS) { return nullptr; }
				if (!__users[idx]) { return nullptr; }
				
				return &(*__users[idx]);
			}
			UserInfo *get_ref(const Shell *shell_ptr) override {
				auto it = std::find_if(__users.begin(), __users.end(), [shell_ptr] (auto &info) -> bool {
					return info
						? info->shell.get() == shell_ptr
						: false;
				});

				if (it == __users.end()) { return nullptr; }
				if (!*it) { return nullptr; }

				return &(**it);
			}

		private:
			inline static const std::string GREETING_MESSAGE = {
				"****************************************\n"
				"** Welcome to the information server. **\n"
				"****************************************\n"
			};
			std::vector<OptionalUserInfo> __users;

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