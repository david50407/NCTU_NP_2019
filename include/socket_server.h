#if !defined(__SOCKET_SERVER_H__)
#define __SOCKET_SERVER_H__

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>

namespace Npshell {
  class SocketServer {
    using Handler = std::function<void (int)>;

    public:
      SocketServer(short, Handler);
      ~SocketServer();
      bool start();
      void close();
    private:
      short _port;
      Handler _handler;
      int _socketFd = -1;
  }; // class SocketServer
}; // namespace Npshell

#endif // !defined(__SOCKET_SERVER_H__)