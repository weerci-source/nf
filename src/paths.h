#pragma once
#include <filesystem>

namespace nf {

std::filesystem::path configDir();     // ~/.config/nf или $XDG_CONFIG_HOME/nf
std::filesystem::path aliasesPath();   // configDir() / "aliases.txt"
std::filesystem::path logPath();       // configDir() / "nf.log"

}  // namespace nf