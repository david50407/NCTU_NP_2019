#include <signal_handler.hxx>

using Npshell::SignalHandler;

std::map<int, std::list<SignalHandler::Callback>> SignalHandler::__subscribers;