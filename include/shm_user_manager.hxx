#if !defined(__SHM_USER_MANAGER_HXX__)
#define __SHM_USER_MANAGER_HXX__

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <atomic>
#include <vector>
#if __cpp_lib_filesystem
	#include <filesystem>
#else
	#include <experimental/filesystem>
#endif
#include <cstring>

#include <util.h>
#include <logger.hxx>
#include <user_manager.h>
#include <signal_handler.hxx>

#define SHM_NAME "/0756125-npshell-shm"
#define FIFO_PATH "user_pipe/"

#if __cpp_lib_filesystem
	namespace fs = std::filesystem;
#else
	namespace fs = std::experimental::filesystem;
#endif

namespace Npshell {
	struct ShmUserInfo {
		int idx;
		pid_t pid;
		char address[16];
		unsigned int port;
		char name[21];
		char message[2049];
		int check_pipe;
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

				::mkdir(FIFO_PATH, 0700);
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
				info.shell->output() << GREETING_MESSAGE << std::flush;
				binded_shell_idx = idx;
				binded_shell = info.shell.get();

				std::stringstream ss;
				ss << "*** User '" << info.name << "' entered from "
					<< info.address << ":" << info.port << ". ***" << std::endl;
				broadcast(ss.str());

				return idx;
			}
			OptionalUserInfo get(int idx) override {
				auto users = list();

				for (auto [idx_, user] : users) {
					if (idx_ == idx) { return std::make_optional(user); }
				}
				return std::nullopt;
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
				for (int i = 1; i != MAX_USERS; ++i) {
					std::stringstream ss;
					ss << FIFO_PATH << idx << "-" << i << ".pipe";
					::unlink(ss.str().c_str());
					
					std::stringstream ss2;
					ss2 << FIFO_PATH << i << "-" << idx << ".pipe";
					::unlink(ss2.str().c_str());
				}
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

			virtual fdpipe createPipe(const int from_, const int to) override {
				std::stringstream ss;
				ss << FIFO_PATH << binded_shell_idx << "-" << to << ".pipe";
				const auto pipe_file = ss.str();

				DBG("Try to create user pipe: " << pipe_file);
				if (fs::exists(pipe_file)) {
					throw already_piped();
				}
				if (auto status = ::mkfifo(pipe_file.c_str(), 0600); status == EEXIST) {
					throw already_piped();
				}

				DBG("Created pipe: " << pipe_file);
				synchronous([&] () {
					shm__->users[to].check_pipe = binded_shell_idx;
				});
				::kill(shm__->users[to].pid, SIGUSR2);

				return fdpipe(
					-1,
					::open(pipe_file.c_str(), O_WRONLY)
				);
			}
			virtual fdpipe removePipe(const int from, const int to_) override {
				auto fd = from_pipe_fds__[from];
				if (fd == -1) {
					throw pipe_not_exists();
				}
				from_pipe_fds__[from] = -1;

				std::stringstream ss;
				ss << FIFO_PATH << from << "-" << binded_shell_idx << ".pipe";

				const auto pipe_path = ss.str();
				if (!fs::exists(pipe_path)) {
					throw pipe_not_exists();
				}
				::unlink(pipe_path.c_str());

				return fdpipe(
					fd,
					-1
				);
			}

			void dispose() {
				if (shm__ != nullptr) { ::munmap(shm__, SHM_SIZE); shm__ = nullptr; }
				if (shmfd__ != -1) { ::close(shmfd__); shmfd__ = -1; }
			}

			static void cleanup_shm() {
				::shm_unlink(SHM_NAME);

				for (int i = 1; i != MAX_USERS; ++i) {
					for (int j = 1; j != MAX_USERS; ++j) {
						std::stringstream ss;
						ss << FIFO_PATH << i << "-" << j << ".pipe";
						::unlink(ss.str().c_str());
					}
				}
			}

		private:
			int shmfd__ = -1;
			ShmContainer *shm__ = nullptr;
			int binded_shell_idx = -1;
			Shell *binded_shell = nullptr;
			std::vector<int> from_pipe_fds__ = {MAX_USERS, -1};

			inline static const auto SHM_SIZE = sizeof(struct ShmContainer) + sizeof(struct ShmUserInfo) * MAX_USERS;

			void synchronous(std::function<void ()> cb) {
				DBG("Waiting for lock, pid: " << getpid());
				while (shm__->lock.test_and_set(std::memory_order_acquire)) ;
				DBG("Win the lock, pid: " << getpid());
				cb();
				shm__->lock.clear(std::memory_order_release);
				DBG("Release lock, pid: " << getpid());
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
				
				SignalHandler::subscribe(SIGUSR2, [&] (const int _signal) {
					if (shm__->users[binded_shell_idx].idx != binded_shell_idx) { return; }
					if (shm__->users[binded_shell_idx].check_pipe == 0) { return; }

					int uid = shm__->users[binded_shell_idx].check_pipe;
					synchronous([&] () {
						shm__->users[binded_shell_idx].check_pipe = 0;
					});

					std::stringstream ss;
					ss << FIFO_PATH << uid << "-" << binded_shell_idx << ".pipe";

					const auto pipe_path = ss.str();
					from_pipe_fds__[uid] = ::open(pipe_path.c_str(), O_RDONLY);
				});
			}
	}; // class ShmUserManager
}; // namespace Npshell

#endif // !defined(__SHM_USER_MANAGER_HXX__)