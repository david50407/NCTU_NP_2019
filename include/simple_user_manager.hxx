#if !defined(__SIMPLE_USER_MANAGER_HXX__)
#define __SIMPLE_USER_MANAGER_HXX__

#include <vector>
#include <optional>
#include <functional>
#include <utility>

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

				return it - __users.begin();
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

		private:
			std::vector<OptionalUserInfo> __users;
	}; // class SimpleUserManager
}; // namespace Npshell

#endif // !defined(__SIMPLE_USER_MANAGER_HXX__)