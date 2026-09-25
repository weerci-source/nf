#pragma once

#include <string>

namespace nf {

std::string toUtf8(const std::wstring& w);
std::wstring fromUtf8(const std::string& s);

} // namespace 