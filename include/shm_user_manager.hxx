#if !defined(__SHM_USER_MANAGER_HXX__)
#define __SHM_USER_MANAGER_HXX__

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <atomic>
#include <cstring>

#include <util.h>
#include <logger.hxx>
#include <user_manager.h>
#include <signal_handler.hxx>

#define SHM_NAME "/0756125-npshell-shm"

namespace Npshell {
	struct ShmUserInfo {
		int idx;
		pid_t pid;
		char address[16];
		unsigned int port;
		char name[21];
		char message[2049];
	}; // struct ShmUserInfo

	struct ShmContainer {
		std::atomic_flag lock = ATOMIC_FLAG_INIT;
		struct ShmUserInfo users[];
	}; // struct ShmContainer

	class ShmUserManager : public UserManager {
		public:
			ShmUserManager() {
				shmfd__ = ::shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);

				if (shmfd__ < 0) {
					DBG("Create SHM failed: " << std::strerror(errno));
					shmfd__ = -1;
				}

				::ftruncate(shmfd__, SHM_SIZE);

				shm__ = (struct ShmContainer *)
					::mmap(nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd__, 0);
			}
			~ShmUserManager() {
				dispose();
			}
			
		public:
			int insert(UserInfo info) override {
				setup_signal();
				int idx = 1;

				synchronous([&] () {
					for (; idx != MAX_USERS; ++idx) {
						if (shm__->users[idx].idx == 0) {
							break;
						}
					}

					auto &user = shm__->users[idx];
					user.idx = idx;
					user.pid = ::getpid();
					::strncpy(user.address, info.address.c_str(), sizeof(user.address));
					user.port = info.port;
					::strncpy(user.name, info.name.c_str(), sizeof(user.name));
					user.message[0] = '\0';
				});

				info.shell->bind_to_user_manager(std::make_shared<Binder>(idx, this));
				info.shell->output() << GREETING_MESSAGE << std::endl;
				binded_shell_idx = idx;
				binded_shell = info.shell.get();

				std::stringstream ss;
				ss << "*** User '" << info.name << "' entered from "
					<< info.address << ":" << info.port << ". ***" << std::endl;
				broadcast(ss.str());

				return idx;
			}
			OptionalUserInfo get(int idx) override {
			}
			bool remove(int idx) override {
				std::string username = shm__->users[idx].name;

				synchronous([&] () {
					shm__->users[idx].idx = 0;
				});

				std::stringstream ss;
				ss << "*** User '" << username << "' left. ***" << std::endl;

				broadcast(ss.str());

				// Cleanup pipes
			}
			std::list<std::pair<int, UserInfo>> list() const override {
				std::list<std::pair<int, UserInfo>> list;

				for (int idx = 1; idx != MAX_USERS; ++idx) {
					if (shm__->users[idx].idx != idx) { continue; }

					auto &user = shm__->users[idx];
					list.emplace_back(std::make_pair(
						idx,
						UserInfo{
							user.address,
							user.port,
							nullptr,
							user.name
						}
					));
				}

				return list;
			}
			
			void broadcast(const std::string msg) {
				synchronous([&] () {
					for (int idx = 1; idx != MAX_USERS; ++idx) {
						if (shm__->users[idx].idx != idx) { continue; }
						if (idx == binded_shell_idx) { continue; }

						::strncpy(shm__->users[idx].message, msg.c_str(), sizeof(ShmUserInfo::message));
					}
				});

				for (int idx = 1; idx != MAX_USERS; ++idx) {
					if (shm__->users[idx].idx != idx) { continue; }
					if (idx == binded_shell_idx) { continue; }
					::kill(shm__->users[idx].pid, SIGUSR1);
				}

				binded_shell->output() << msg << std::flush;
			}
			bool tell(const int idx, const std::string msg) {
				if (shm__->users[idx].idx != idx) {
					return false;
				}
				if (idx == binded_shell_idx) {
					binded_shell->output() << msg << std::flush;
					return true;
				}

				synchronous([&] () {
					::strncpy(shm__->users[idx].message, msg.c_str(), sizeof(ShmUserInfo::message));
				});
				::kill(shm__->users[idx].pid, SIGUSR1);
			}
			bool rename(const int idx, const std::string name) {
				for (int i = 1; i != MAX_USERS; ++i) {
					if (shm__->users[i].idx != i) { continue; }

					if (name == shm__->users[i].name) {
						std::stringstream ss;
						ss << "*** User '" << name << "' already exists. ***" << std::endl;
						tell(idx, ss.str());

						return false;
					}
				}

				synchronous([&] () {
					::strncpy(shm__->users[idx].name, name.c_str(), sizeof(ShmUserInfo::name));
				});

				std::stringstream ss;
				ss << "*** User from " << shm__->users[idx].address
					<< ":" << shm__->users[idx].port << " is named '" << name << "'. ***"
					<< std::endl;
				broadcast(ss.str());

				return true;
			}

			virtual fdpipe createPipe(const int from, const int to) override {
				throw;
			}
			virtual fdpipe removePipe(const int from, const int to) override {
				throw;
			}

			void dispose() {
				if (shm__ != nullptr) { ::munmap(shm__, SHM_SIZE); shm__ = nullptr; }
				if (shmfd__ != -1) { ::close(shmfd__); shmfd__ = -1; }
			}

			static void cleanup_shm() {
				::shm_unlink(SHM_NAME);
			}

		private:
			int shmfd__ = -1;
			ShmContainer *shm__ = nullptr;
			int binded_shell_idx = -1;
			Shell *binded_shell = nullptr;

			inline static const auto SHM_SIZE = sizeof(struct ShmContainer) + sizeof(struct ShmUserInfo) * MAX_USERS;

			void synchronous(std::function<void ()> cb) {
				while (shm__->lock.test_and_set()) ;
				cb();
				shm__->lock.clear();
			}

			void setup_signal() {
				SignalHandler::subscribe(SIGUSR1, [&] (const int _signal) {
					if (shm__->users[binded_shell_idx].idx != binded_shell_idx) { return; }
					if (shm__->users[binded_shell_idx].message[0] == '\0') { return; }

					std::string msg = shm__->users[binded_shell_idx].message;
					synchronous([&] () {
						shm__->users[binded_shell_idx].message[0] = '\0';
					});

					binded_shell->output() << msg << std::flush;
				});
			}
	}; // class ShmUserManager
}; // namespace Npshell

#endif // !defined(__SHM_USER_MANAGER_HXX__)