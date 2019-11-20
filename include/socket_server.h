#if !defined(__SOCKET_SERVER_H__)
#define __SOCKET_SERVER_H__

#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>
#include <optional>
#include <list>
#include <set>

namespace Npshell {
  struct ClientInfo {
    std::string address;
    unsigned short port;

    public:
      ClientInfo(struct sockaddr_in addr)
        : address(), port(::ntohs(addr.sin_port)) {
        char c_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), c_str, INET_ADDRSTRLEN);

        address.assign(c_str);
      }
  }; // struct ClientInfo

  class SocketServer {
    using Handler = std::function<bool (int, std::optional<ClientInfo>)>;

    public:
      SocketServer(short, Handler);
      ~SocketServer();
      bool start();
      void close();
      void set_nonblocking(bool flag) {
        _nonblocking = flag;
      }
    private:
      short _port;
      Handler _handler;
      int _socketFd = -1;
      bool _nonblocking = false;
      std::set<int> _clientFds;
    private:
      void handle_connection(int, std::optional<ClientInfo>);
      std::optional<std::pair<int, struct sockaddr_in>> accept_connection();
      std::list<int> get_select_fds();
  }; // class SocketServer
}; // namespace Npshell

#endif // !defined(__SOCKET_SERVER_H__)