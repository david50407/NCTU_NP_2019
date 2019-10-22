#include <string>
#include <iostream>
#include <initializer_list>

#include <command.h>
#include <util.h>
#include <logger.hxx>

using Npshell::Command;

Command::Command() : Command({}) {}
Command::Command(const std::initializer_list<std::string> args) :
	args(args), redirect_out(""), pipe_to_n(-1) {}

Command::~Command() {}

std::list<Command::Chain> Command::parse_commands(const std::string &str) {
	std::list<Command::Chain> chains = { { Command() } };
	Command::Cmdlist args = { "" };
	
	// Parse args
	for (auto it = str.begin(); it != str.end(); ++it) {
		auto &now = args.back();
		switch (*it) {
			case ' ': // split command arguments
				if (now.size() > 0) {
					args.emplace_back(std::string());
				}
				break;
			case '>':
			case '|':
				if (now.size() > 0) {
					args.emplace_back(std::string());
				}
				args.back() += *it;
				break;
			default:
				if (!now.empty() && now[0] == '|') { // Pipe to n-th command
					if (*it < '0' || '9' < *it) {
						goto syntax_error;
					} 
				}
				now += *it;
		}
	}
	if (args.back().empty()) {
		args.pop_back();
	}
	
	// Parse commands
	for (auto it = args.begin(); it != args.end(); ++it) {
		auto &chain = chains.back();
		auto &now = chain.back();
		if (it->size() == 1 && (*it)[0] == '>') {
			if (now.args.size() == 0)
				goto syntax_error;
			if (++it == args.end())
				goto syntax_error;
			if (now.redirect_out.size() > 0 )
				goto syntax_error;
			now.redirect_out = std::string(*it);
			continue;
		}
		if (it->size() == 1 && (*it)[0] == '|') {
			if (now.args.size() == 0)
				goto syntax_error;
			if (++it == args.end())
				goto syntax_error;
			chain.emplace_back(Command({*it}));
			continue;
		}
		if ((*it)[0] == '|') {
			now.pipe_to_n = ::atoi(it->c_str() + 1);
			chains.emplace_back(Command::Chain{ Command() });
			continue;
		}
		if (now.redirect_out.size() > 0)
			goto syntax_error;
		now.args.push_back(*it);
	}
	for (auto &chain : chains) {
		if (chain.back().args.empty()) {
			chain.pop_back();
		}
	}
	chains.remove_if([](auto &chain) {
		return chain.empty();
	});

	return chains;

syntax_error: ;
	return {{
		Command({"$__error", "Syntax error."})
	}};
}