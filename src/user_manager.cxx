#include <sstream>

#include <user_manager.h>
#include <shell.h>
#include <logger.hxx>

using Npshell::Shell;
using Npshell::UserManager;
using Npshell::UserInfo;

const std::string UserManager::GREETING_MESSAGE = {
	"****************************************\n"
	"** Welcome to the information server. **\n"
	"****************************************\n"
};