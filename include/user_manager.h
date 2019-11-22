#if !defined(__USER_MANAGER_H__)
#define __USER_MANAGER_H__

#include <optional>
#include <memory>
#include <functional>
#include <list>
#include <utility>
#include <sstream>

namespace Npshell {
	class Shell;
	
	struct UserInfo {
		std::string address;
		unsigned int port;
		std::shared_ptr<Shell> shell;
		std::string name = "(no name)";
	}; // struct UserInfo

	class UserManager {
		public:
			struct Binder;

		protected:
			using OptionalUserInfo = std::optional<UserInfo>;
			static const int MAX_USERS = 31;

		public:
			virtual int insert(UserInfo) = 0;
			virtual OptionalUserInfo get(int) = 0;
			virtual OptionalUserInfo get(const Shell *) = 0;
			virtual bool remove(int) = 0;
			virtual const std::list<std::pair<int, const std::reference_wrapper<const UserInfo>>> list() const = 0;

			virtual void broadcast(const std::string);
			virtual bool tell(const int, const std::string);
			virtual bool rename(const int, const std::string);

		protected:
			virtual UserInfo *get_ref(int) = 0;
			virtual UserInfo *get_ref(const Shell *) = 0;
	}; // class UserManager

	struct UserManager::Binder {
		public:
			Binder(const int idx, UserManager *um)
				: idx_(idx), um_(um) {}
			inline const int id() { return idx_; }
			inline UserManager* user_manager() { return um_; }
			inline const decltype(std::declval<UserManager>().list()) list() { return um_->list(); }
			inline const void broadcast(const std::string msg) {
				const auto from = um_->get(idx_);

				std::stringstream ss;
				ss << "*** " << from->name << " yelled ***: " << msg << std::endl;

				um_->broadcast(ss.str());
			}
			inline const bool tell(const int who_idx, const std::string msg) {
				const auto from = um_->get(idx_);

				std::stringstream ss;
				ss << "*** " << from->name << " told you ***: " << msg << std::endl;

				return um_->tell(who_idx, ss.str());
			}
			inline const bool rename(const std::string name) {
				return um_->rename(idx_, name);
			}

		private:
			const int idx_;
			UserManager *um_;

		friend class UserManager;
	}; // struct UserManager::Binder
}; // namespace Npshell

#endif // !defined(__USER_MANAGER_H__)