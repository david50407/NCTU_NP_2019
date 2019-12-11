#include <string>
#include <iostream>
#include <initializer_list>

#include <command.h>
#include <util.h>
#include <logger.hxx>

using Npshell::Command;

Command::Command() : Command{std::initializer_list<std::string>{}} {}
Command::Command(const std::initializer_list<std::string> args) :
	args(args) {}

Command::~Command() {}

std::list<Command::Chain>
Command::parse_commands(const std::string &str, const bool multi_user) {
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
			case '<':
			case '>':
				if (!multi_user) {
					goto default__;
				}
			case '|':
			case '!':
				if (now.size() > 0) {
					args.emplace_back(std::string());
				}
				args.back() += *it;
				break;
			default:
			default__: ;
				if (!now.empty() && (
						now[0] == '|' || now[0] == '!' // Pipe to n-th command
						|| (multi_user && (now[0] == '<' || now[0] == '>')) // Pipe to/from user
					)) {
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
		switch (auto pipe_type = (*it)[0]; pipe_type) {
			case '!':
			case '|':
				now.pipe_to_n = ::atoi(it->c_str() + 1);
				now.pipe_stderr = pipe_type == '!';
				chains.emplace_back(Command::Chain{ Command() });
				continue;
			case '>':
				now.pipe_to_uid = ::atoi(it->c_str() + 1);
				continue;
			case '<':
				now.pipe_from_uid = ::atoi(it->c_str() + 1);
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
