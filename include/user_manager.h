#if !defined(__USER_MANAGER_H__)
#define __USER_MANAGER_H__

#include <optional>
#include <memory>
#include <functional>
#include <list>
#include <utility>

#include <shell.h>

namespace Npshell {
	struct UserInfo {
		std::string address;
		unsigned int port;
		std::shared_ptr<Shell> shell;
		std::string name = "(no name)";
	}; // struct UserInfo

	class UserManager {
		protected:
			using OptionalUserInfo = std::optional<UserInfo>;
			static const int MAX_USERS = 31;

		public:
			virtual int insert(UserInfo) = 0;
			virtual OptionalUserInfo get(int) = 0;
			virtual bool remove(int) = 0;
			virtual const std::list<std::pair<int, const std::reference_wrapper<const UserInfo>>> list() const = 0;
			virtual void tell(const std::string msg, std::initializer_list<int> ids) {
				for (auto idx : ids) {
					if (auto user = get(idx); user) {
						user->shell->output() << msg << std::flush;
					}
				}
			}

			void broadcast(const std::string msg) {
				for (auto [idx, user_ref] : list()) {
					user_ref.get().shell->output() << msg << std::flush;
				}
			}
	}; // class UserManager
}; // namespace Npshell

#endif // !defined(__USER_MANAGER_H__)