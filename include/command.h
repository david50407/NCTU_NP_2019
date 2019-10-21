#if !defined(__COMMAND_H__)
#define __COMMAND_H__

#include <vector>
#include <string>

namespace Npshell {
  class Command {
    public:
      using Cmdlist = std::vector<std::string>;

    public:
      static bool execute(const Cmdlist);
    private:
      static bool cmd_exit(const Cmdlist &); 
      static bool cmd_execute(const Cmdlist &);
  };
}; // namespace Npshell

#endif // !defined(__COMMAND_H__)