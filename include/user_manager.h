#if !defined(__USER_MANAGER_H__)
#define __USER_MANAGER_H__

#include <optional>
#include <memory>
#include <functional>
#include <list>
#include <utility>

namespace Npshell {
	class Shell;
	
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
			virtual OptionalUserInfo get(const Shell *) = 0;
			virtual bool remove(int) = 0;
			virtual const std::list<std::pair<int, const std::reference_wrapper<const UserInfo>>> list() const = 0;

			virtual bool tell(const std::string, const int);
			virtual bool tell(const std::string, const int, const Shell *);
			virtual void broadcast(const std::string);
			virtual void broadcast(const std::string, const Shell *);
			virtual void rename(const Shell *, const std::string);

		protected:
			virtual UserInfo *get_ref(int) = 0;
			virtual UserInfo *get_ref(const Shell *) = 0;
	}; // class UserManager
}; // namespace Npshell

#endif // !defined(__USER_MANAGER_H__)