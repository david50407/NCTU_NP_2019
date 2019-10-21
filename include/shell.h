#if !defined(__SHELL_H__)
#define __SHELL_H__

#include <string>

namespace Npshell {
  class Shell {
    public:
      Shell();
      void run();

    private:
      void show_prompt();
      std::string read_command();
      bool parse_command(std::string);
  };
}; // namespace Npshell

#endif // !defined(__SHELL_H__)