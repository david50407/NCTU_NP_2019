#if !defined(__USER_MANAGER_H__)
#define __USER_MANAGER_H__

#include <optional>
#include <memory>
#include <functional>

#include <shell.h>

namespace Npshell {
	struct UserInfo {
		std::string address;
		int port;
		std::shared_ptr<Shell> shell;
	}; // struct UserInfo

	class UserManager {
		protected:
			using OptionalUserInfo = std::optional<UserInfo>;
			static const int MAX_USERS = 31;

		public:
			virtual int insert(UserInfo) = 0;
			virtual OptionalUserInfo get(int) = 0;
			virtual bool remove(int) = 0;
	}; // class UserManager
}; // namespace Npshell

#endif // !defined(__USER_MANAGER_H__)