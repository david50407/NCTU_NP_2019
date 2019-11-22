#include <user_manager.h>
#include <shell.h>

using Npshell::Shell;
using Npshell::UserManager;
using Npshell::UserInfo;

bool UserManager::tell(const std::string msg, std::initializer_list<int> ids) {
  bool success = false;
  for (auto idx : ids) {
    if (auto user = get(idx); user) {
      user->shell->output() << msg << std::flush;
      success = true;
    }
  }

  return success;
}

void UserManager::broadcast(const std::string msg) {
  for (auto [idx, user_ref] : list()) {
    user_ref.get().shell->output() << msg << std::flush;
  }
}