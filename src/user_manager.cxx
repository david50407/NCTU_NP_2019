#include <sstream>

#include <user_manager.h>
#include <shell.h>
#include <logger.hxx>

using Npshell::Shell;
using Npshell::UserManager;
using Npshell::UserInfo;

bool UserManager::tell(const int idx, const std::string msg) {
	if (auto user = get(idx); user) {
		user->shell->output() << msg << std::flush;
		return true;
	}

	return false;
}

void UserManager::broadcast(const std::string msg) {
	for (auto [idx, user_ref] : list()) {
		user_ref.get().shell->output() << msg << std::flush;
	}
}

bool UserManager::rename(const int idx, const std::string name) {
	auto user_ref = get_ref(idx);
	
	if (user_ref == nullptr) {
		DBG("Shell not managed");
		return true;
	}
	
	const auto users = list();
	if (const auto it = std::find_if(
			users.begin(), users.end(),
			[name] (auto &pair) { return pair.second.get().name == name; }
		); it != users.end()) {
		user_ref->shell->error()
			<< "*** User '" << name << "' already exists. ***" << std::endl;

		return false;
	}

	user_ref->name = name;

	std::stringstream ss;
	ss << "*** User from " << user_ref->address
		<< ":" << user_ref->port << " is named '" << name << "'. ***"
		<< std::endl;
	broadcast(ss.str());

	return true;
}