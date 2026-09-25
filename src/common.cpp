#include "common.h"

#include "WideMB.h"

namespace nf {

std::string toUtf8(const std::wstring& w) {
    return Wide2MB(w.c_str());
}

std::wstring fromUtf8(const std::string& s) {
    return MB2Wide(s.c_str());
}

} // namespace nf