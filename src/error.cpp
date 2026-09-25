#include "error.h"
#include "common.h"

namespace nf {

// GetMsg объявлен в PluginContext.h, но чтобы error.cpp не тянул far2l
// SDK в тесты — оставляем только forward declaration.
const wchar_t* GetMsg(MsgID id);

std::wstring Error::localizedMessage() const {
    return substitutePlaceholders(GetMsg(msg_id), args);
}

std::wstring Error::stackTraceToString() const {
    if (file == nullptr || *file == '\0') {
        return {};
    }
    std::string s =
        std::string(file) + ":" + std::to_string(line) + " in " + (function ? function : "?");
    return std::wstring(s.begin(), s.end());
}

} // namespace nf