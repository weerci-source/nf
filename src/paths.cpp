#include "paths.h"

#include <cstdlib>

namespace nf {

std::filesystem::path configDir() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg != nullptr && *xdg != '\0') {
        return std::filesystem::path(xdg) / "nf";
    }
    const char* home = std::getenv("HOME");
    if (home != nullptr && *home != '\0') {
        return std::filesystem::path(home) / ".config" / "nf";
    }
    return std::filesystem::path(".") / "nf";
}

std::filesystem::path aliasesPath() {
    return configDir() / "aliases.txt";
}
std::filesystem::path logPath() {
    return configDir() / "nf.log";
}

} // namespace nf