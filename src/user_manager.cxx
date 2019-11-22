#include <sstream>

#include <user_manager.h>
#include <shell.h>
#include <logger.hxx>

using Npshell::Shell;
using Npshell::UserManager;
using Npshell::UserInfo;

bool UserManager::tell(const std::string msg, const int idx) {
	if (auto user = get(idx); user) {
		user->shell->output() << msg << std::flush;
		return true;
	}

	return false;
}

bool UserManager::tell(const std::string msg, const int idx, const Shell* shell_ptr) {
	auto from_ref = get_ref(shell_ptr);
	
	if (from_ref == nullptr) {
		DBG("Shell not managed");
		return false;
	}

	if (auto user = get(idx); user) {
		user->shell->output()
			<< "*** " << from_ref->name << " told you ***: "
			<< msg << std::endl;
		return true;
	}

	return false;
}

void UserManager::broadcast(const std::string msg) {
	for (auto [idx, user_ref] : list()) {
		user_ref.get().shell->output() << msg << std::flush;
	}
}

void UserManager::broadcast(const std::string msg, const Shell *shell_ptr) {
	auto from_ref = get_ref(shell_ptr);
	
	if (from_ref == nullptr) {
		DBG("Shell not managed");
		return;
	}

	for (auto [idx, user_ref] : list()) {
		user_ref.get().shell->output()
			<< "*** " << from_ref->name << " yelled ***: "
			<< msg << std::endl;
	}
}

void UserManager::rename(const Shell *shell_ptr, const std::string name) {
	auto user_ref = get_ref(shell_ptr);
	
	if (user_ref == nullptr) {
		DBG("Shell not managed");
		return;
	}
	
	const auto users = list();
	if (const auto it = std::find_if(
			users.begin(), users.end(),
			[name] (auto &pair) { return pair.second.get().name == name; }
		); it != users.end()) {
		user_ref->shell->error()
			<< "*** User '" << name << "' already exists. ***" << std::endl;

		return;
	}

	user_ref->name = name;

	std::stringstream ss;
	ss << "*** User from " << user_ref->address
		<< ":" << user_ref->port << " is named '" << name << "'. ***"
		<< std::endl;
	broadcast(ss.str());
}