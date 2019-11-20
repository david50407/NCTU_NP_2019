#include <cstring>
#include <iostream>
#include <utility>
#include <errno.h>
#include <unistd.h>

#include <logger.hxx>
#include <socket_server.h>

using Npshell::SocketServer;

SocketServer::SocketServer(short port, SocketServer::Handler handler)
	: _port(port), _handler(handler), _clientFds() {}

SocketServer::~SocketServer() {
	close();
}

bool SocketServer::start() {
	_socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFd < 0) {
		DBG("Cannot create socket: " << ::strerror(errno));
		return false;
	}

	if (int sockopt = 1; ::setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
		DBG("Cannot set socket option: " << ::strerror(errno));
		return false;
	}

	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = ::htons(_port);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	if (::bind(_socketFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
		DBG("Cannot bind to socket: " << ::strerror(errno));
		return false;
	}

	::listen(_socketFd, SOMAXCONN);
	
	while (true) {
		for (auto fd : get_select_fds()) {
			if (fd == _socketFd) { // New client
				auto clientInfo = accept_connection();
				if (!clientInfo) { continue; }

				auto [clientFd, clientAddr] = *clientInfo;
				_clientFds.insert(clientFd);
				handle_connection(clientFd);
			} else {
				handle_connection(fd);
			}
		}
	}

	return true;
}

void SocketServer::close() {
	if (_socketFd != -1) {
		::close(_socketFd);
		_socketFd = -1;
	}
}

void SocketServer::handle_connection(int fd) {
	if (_handler(fd)) {
		::close(fd);
		_clientFds.erase(fd);
	}
}

std::optional<std::pair<int, struct sockaddr_in>> SocketServer::accept_connection() {
	struct sockaddr_in sa;
	socklen_t saLen = sizeof(sa);
	int fd = ::accept(_socketFd, reinterpret_cast<sockaddr *>(&sa), &saLen);

	return fd < 0
		? std::nullopt
		: std::make_optional(std::make_pair(fd, sa))
		;
}

std::list<int> SocketServer::get_select_fds() {
	if (!_nonblocking) { return { _socketFd }; }

	int nfds = _socketFd;
	fd_set fdset; FD_ZERO(&fdset);

	FD_SET(_socketFd, &fdset);
	for (auto fd : _clientFds) {
		FD_SET(fd, &fdset);
		if (fd > nfds) { nfds = fd; }
	}

	if (::select(nfds + 1, &fdset, nullptr, nullptr, nullptr) < 0) {
		DBG("Cannot select: " << std::strerror(errno));
		return {};
	}

	std::list<int> fds;
	if (FD_ISSET(_socketFd, &fdset)) {
		fds.push_back(_socketFd);
	}

	for (auto fd : _clientFds) {
		if (FD_ISSET(fd, &fdset)) {
			fds.push_back(fd);
		}
	}

	return fds;
}