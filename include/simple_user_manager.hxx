#if !defined(__SIMPLE_USER_MANAGER_HXX__)
#define __SIMPLE_USER_MANAGER_HXX__

#include <vector>
#include <optional>
#include <functional>
#include <utility>
#include <sstream>
#include <type_traits>

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

				*it = std::nullopt;
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

		private:
			inline static const std::string GREETING_MESSAGE = {
				"****************************************\n"
				"** Welcome to the information server. **\n"
				"****************************************\n"
			};
			std::vector<OptionalUserInfo> __users;

		private:
			void greeting(int idx) {
				if (auto user = get(idx); user) {
					user->shell->output() << GREETING_MESSAGE << std::flush;

					std::stringstream ss;
					ss << "*** User '" << user->name << "' entered from "
						<< user->address << ":" << user->port << ". ***" << std::endl;

					broadcast(ss.str());
				}
			}
	}; // class SimpleUserManager
}; // namespace Npshell

#endif // !defined(__SIMPLE_USER_MANAGER_HXX__)