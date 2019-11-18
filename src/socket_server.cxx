#include <cstring>
#include <iostream>
#include <errno.h>
#include <unistd.h>

#include <logger.hxx>
#include <socket_server.h>

using Npshell::SocketServer;

SocketServer::SocketServer(short port, SocketServer::Handler handler)
	: _port(port), _handler(handler) {}

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

	int clientFd = -1;
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	
	while (true) {
		clientFd = ::accept(_socketFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientAddrLen);
		if (clientFd < 0) { continue; }
		_handler(clientFd);
	}

	return true;
}

void SocketServer::close() {
	if (_socketFd != -1) {
		::close(_socketFd);
		_socketFd = -1;
	}
}