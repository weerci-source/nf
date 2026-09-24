#include "error.h"

namespace nf {

// GetMsg объявлен в PluginContext.h, но чтобы error.cpp не тянул far2l
// SDK в тесты — оставляем только forward declaration.
const wchar_t* GetMsg(MsgID id);

std::wstring Error::localizedMessage() const {
    std::wstring msg = GetMsg(msg_id);
    for (size_t i = 0; i < args.size(); ++i) {
        const std::wstring placeholder = L"{" + std::to_wstring(i) + L"}";
        size_t pos = 0;
        while ((pos = msg.find(placeholder, pos)) != std::wstring::npos) {
            msg.replace(pos, placeholder.length(), args[i]);
            pos += args[i].length();
        }
    }
    return msg.empty() ? L"<untranslated>" : msg;
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