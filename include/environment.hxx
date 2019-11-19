#if !defined(__ENVIRONMENT_H__)
#define __ENVIRONMENT_H__

#include <unordered_map>
#include <string>
#include <optional>
#include <utility>
#include <initializer_list>

namespace Npshell {
	class Environment {
		using HashMap = std::unordered_map<std::string, std::string>;

		public:
			Environment() : __envs() {};
			Environment(const Environment &env) : __envs(env.__envs) {}
			Environment(const std::initializer_list<HashMap::value_type> envs)
				: __envs(envs) {}
			Environment(const char **envs) : __envs() {
				for (int idx = 0; envs[idx]; ++idx) {
					std::string env(envs[idx]);
					if (auto delimiter = env.find("="); delimiter != std::string::npos) {
						__envs[env.substr(0, delimiter)] = env.substr(delimiter + 1);
					} else {
						__envs[env] = "";
					}
				}
			}

		public:
			std::optional<std::string> get(const std::string key) const {
				if (auto env = __envs.find(key); env == __envs.end()) {
					return std::nullopt;
				} else {
					return env->second;
				}
			}
			Environment &set(const std::string key, const std::string value) {
				__envs[key] = value;

				return *this;
			}
			const HashMap map() const {
				return __envs;
			}

		private:
			HashMap __envs;
	};
}; // namespace Npshell

#endif // !defined(__ENVIRONMENT_H__)