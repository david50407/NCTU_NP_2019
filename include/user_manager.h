#if !defined(__USER_MANAGER_H__)
#define __USER_MANAGER_H__

#include <optional>
#include <memory>
#include <functional>
#include <list>
#include <utility>
#include <sstream>
#include <exception>

#include <fdpipe.hxx>

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
			class already_piped : public std::exception {
				public:
					const char* what() const throw () override { return "Already piped with target"; }
			}; // class already_piped
			class pipe_not_exists : public std::exception {
				public:
					const char* what() const throw () override { return "Not piped with sender yet"; }
			}; // class pipe_not_exists
			class user_not_exists : public std::exception {
				public:
					const char* what() const throw () override { return "User not exists"; }
			}; // class user_not_exists

		protected:
			using OptionalUserInfo = std::optional<UserInfo>;
			static const int MAX_USERS = 31;
			static const std::string GREETING_MESSAGE;

		public:
			virtual int insert(UserInfo) = 0;
			virtual OptionalUserInfo get(int) = 0;
			virtual bool remove(int) = 0;
			virtual std::list<std::pair<int, UserInfo>> list() const = 0;

			virtual void broadcast(const std::string) = 0;
			virtual bool tell(const int, const std::string) = 0;
			virtual bool rename(const int, const std::string) = 0;

			virtual fdpipe createPipe(const int, const int) = 0;
			virtual fdpipe removePipe(const int, const int) = 0;
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
			inline const int pipeTo(const int idx, const std::string command = "") {
				auto user = um_->get(idx); 
				if (!user) {
					throw user_not_exists();
				}

				auto pipe = um_->createPipe(idx_, idx);

				std::stringstream ss;
				ss << "*** " << um_->get(idx_)->name
					<< " (#" << idx_ << ") just piped '"
					<< command << "' to " << user->name
					<< " (#" << idx << ") ***"
					<< std::endl;
				um_->broadcast(ss.str());

				return pipe.output();
			}
			inline const int pipeFrom(const int idx, const std::string command = "") {
				auto user = um_->get(idx);
				if (!user) {
					throw user_not_exists();
				}

				auto pipe = um_->removePipe(idx, idx_);

				if (command.size() > 0) {
					std::stringstream ss;
					ss << "*** " << um_->get(idx_)->name
						<< " (#" << idx_ << ") just received from " << user->name
						<< " (#" << idx << ") by '" << command << "' ***"
						<< std::endl;
					um_->broadcast(ss.str());
				}

				return pipe.input();
			}

		private:
			const int idx_;
			UserManager *um_;

		friend class UserManager;
	}; // struct UserManager::Binder
}; // namespace Npshell

#endif // !defined(__USER_MANAGER_H__)