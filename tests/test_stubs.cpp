#include "msg_ids.h"

namespace nf {

// Заглушка для линковки error.cpp в тестах.
// Тесты не проверяют localizedMessage — только stackTraceToString
// и substitutePlaceholders (которая теперь живёт в common.cpp).
const wchar_t* GetMsg(MsgID /*id*/) {
    return L"<stub>";
}

} // namespace nf