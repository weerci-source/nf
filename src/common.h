#pragma once

#include <string>
#include <vector>

namespace nf {

std::string toUtf8(const std::wstring& w);
std::wstring fromUtf8(const std::string& s);
std::wstring substitutePlaceholders(std::wstring tmpl, const std::vector<std::wstring>& args);

} // namespace nf