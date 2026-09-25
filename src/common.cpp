#include "common.h"

#include <cstdint>

namespace nf {

namespace {

// wchar_t на Linux = UTF-32. На Windows был бы UTF-16, но плагин под far2l
// работает только на Linux/BSD/macOS, где wchar_t = UTF-32.

void appendUtf8(std::string& out, char32_t cp) {
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    // вне диапазона — молча пропускаем
}

} // namespace

std::string toUtf8(const std::wstring& w) {
    std::string out;
    out.reserve(w.size() * 2); // грубая, но разумная оценка для кириллицы
    for (wchar_t wc : w) {
        appendUtf8(out, static_cast<char32_t>(wc));
    }
    return out;
}

std::wstring fromUtf8(const std::string& s) {
    std::wstring out;
    out.reserve(s.size());

    const auto* p = reinterpret_cast<const unsigned char*>(s.data());
    const auto* end = p + s.size();

    while (p < end) {
        char32_t cp = 0;
        int extra = 0;

        if (*p < 0x80) {
            cp = *p;
            extra = 0;
        } else if ((*p & 0xE0) == 0xC0) {
            cp = *p & 0x1F;
            extra = 1;
        } else if ((*p & 0xF0) == 0xE0) {
            cp = *p & 0x0F;
            extra = 2;
        } else if ((*p & 0xF8) == 0xF0) {
            cp = *p & 0x07;
            extra = 3;
        } else {
            // невалидный стартовый байт — пропускаем
            ++p;
            continue;
        }

        if (p + extra >= end) break; // обрезанная последовательность

        bool ok = true;
        for (int k = 1; k <= extra; ++k) {
            if ((p[k] & 0xC0) != 0x80) { ok = false; break; }
            cp = (cp << 6) | (p[k] & 0x3F);
        }
        if (!ok) { ++p; continue; }

        out.push_back(static_cast<wchar_t>(cp));
        p += extra + 1;
    }
    return out;
}

} // namespace nf