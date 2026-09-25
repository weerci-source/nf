#include <catch2/catch_test_macros.hpp>

#include "common.h"

using nf::fromUtf8;
using nf::substitutePlaceholders;
using nf::toUtf8;

// ==================== UTF-8 codec ====================
// Именно тут был краш: Wide2MB/MB2Wide из far2l не линковались.
// Проверяем ручной кодек на всех классах последовательностей.

TEST_CASE("toUtf8: пустая строка", "[common][utf8]") {
    CHECK(toUtf8(L"") == "");
}

TEST_CASE("toUtf8: ASCII", "[common][utf8]") {
    CHECK(toUtf8(L"hello") == "hello");
    CHECK(toUtf8(L"nf") == "nf");
    CHECK(toUtf8(L"a-b_c.d") == "a-b_c.d");
}

TEST_CASE("toUtf8: кириллица (2 байта)", "[common][utf8]") {
    // "Привет" = 6 кодпоинтов * 2 байта
    const auto out = toUtf8(L"Привет");
    CHECK(out.size() == 12);
    CHECK(out == "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82");
}

TEST_CASE("toUtf8: CJK (3 байта)", "[common][utf8]") {
    // "日本" = 2 кодпоинта * 3 байта
    CHECK(toUtf8(L"日本") == "\xE6\x97\xA5\xE6\x9C\xAC");
}

TEST_CASE("toUtf8: emoji (4 байта)", "[common][utf8]") {
    // U+1F600 GRINNING FACE
    CHECK(toUtf8(L"\U0001F600") == "\xF0\x9F\x98\x80");
    CHECK(toUtf8(L"\U0001F389") == "\xF0\x9F\x8E\x89"); // 🎉
}

TEST_CASE("fromUtf8: пустая строка", "[common][utf8]") {
    CHECK(fromUtf8("") == L"");
}

TEST_CASE("fromUtf8: ASCII", "[common][utf8]") {
    CHECK(fromUtf8("hello") == L"hello");
}

TEST_CASE("fromUtf8: 2/3/4-байтовые последовательности", "[common][utf8]") {
    CHECK(fromUtf8("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82") == L"Привет");
    CHECK(fromUtf8("\xE6\x97\xA5\xE6\x9C\xAC") == L"日本");
    CHECK(fromUtf8("\xF0\x9F\x98\x80") == L"\U0001F600");
}

TEST_CASE("fromUtf8: невалидные последовательности не крашат", "[common][utf8]") {
    // Отрезанный continuation byte
    CHECK_NOTHROW(fromUtf8("\x80"));
    // Continuation без старта
    CHECK_NOTHROW(fromUtf8("\x80\x80\x80"));
    // Обрезанная 3-байтовая
    CHECK_NOTHROW(fromUtf8("\xE6\x97"));
    // Случайные байты
    CHECK_NOTHROW(fromUtf8(std::string("\xFF\xFE", 2)));
    // Строка из продолжений после валидного старта
    CHECK_NOTHROW(fromUtf8("a\xC3\x28")); // невалидный continuation
}

TEST_CASE("UTF-8: round-trip на представительном наборе", "[common][utf8]") {
    const std::wstring samples[] = {
        L"",
        L"abc",
        L"Привет, мир!",
        L"日本語テスト",
        L"emoji: \U0001F389\U0001F525\U0001F4A1",
        L"Смесь: nf — 日本 — \U0001F389",
        L"Ёлка, ё-моё",
    };
    for (const auto& s : samples) {
        CHECK(fromUtf8(toUtf8(s)) == s);
    }
}

// ==================== substitutePlaceholders ====================
// Именно тут был тихий баг: sequential replace ломался, если
// аргумент сам содержал {N}. Тесты фиксируют правильное поведение.

TEST_CASE("substitutePlaceholders: без аргументов", "[common][format]") {
    CHECK(substitutePlaceholders(L"hello", {}) == L"hello");
    CHECK(substitutePlaceholders(L"", {}) == L"");
    CHECK(substitutePlaceholders(L"{0}", {}) == L"{0}"); // нет аргумента — оставляем
}

TEST_CASE("substitutePlaceholders: один аргумент", "[common][format]") {
    CHECK(substitutePlaceholders(L"Hi, {0}!", {L"World"}) == L"Hi, World!");
}

TEST_CASE("substitutePlaceholders: несколько аргументов", "[common][format]") {
    CHECK(substitutePlaceholders(L"{0} + {1} = {2}", {L"1", L"2", L"3"}) == L"1 + 2 = 3");
}

TEST_CASE("substitutePlaceholders: повторяющийся placeholder", "[common][format]") {
    CHECK(substitutePlaceholders(L"{0}{0}{0}", {L"x"}) == L"xxx");
}

TEST_CASE("substitutePlaceholders: аргумент с {N} НЕ разворачивается рекурсивно",
          "[common][format][regression]") {
    // Регрессия на баг с AliasCreated: "Alias '{0}' -> {1}", где {0}=""
    // содержит "{1}". Правильное поведение: '{1}' остаётся как есть.
    CHECK(substitutePlaceholders(L"{0} {1}", {L"{1}", L"y"}) == L"{1} y");
    CHECK(substitutePlaceholders(L"alias '{0}' -> {1}", {L"{1}", L"/tmp"}) ==
          L"alias '{1}' -> /tmp");
}

TEST_CASE("substitutePlaceholders: невалидные placeholder'ы остаются", "[common][format]") {
    CHECK(substitutePlaceholders(L"{", {}) == L"{");
    CHECK(substitutePlaceholders(L"{}", {L"x"}) == L"{}");
    CHECK(substitutePlaceholders(L"{abc}", {L"x"}) == L"{abc}");
    CHECK(substitutePlaceholders(L"{0", {L"x"}) == L"{0");
    CHECK(substitutePlaceholders(L"0}", {L"x"}) == L"0}");
}

TEST_CASE("substitutePlaceholders: индекс вне диапазона", "[common][format]") {
    CHECK(substitutePlaceholders(L"{5}", {L"a", L"b"}) == L"{5}");
}

TEST_CASE("substitutePlaceholders: реальный сценарий nf", "[common][format]") {
    // "Alias '{0}' -> {1}"
    CHECK(substitutePlaceholders(L"Alias '{0}' -> {1}", {L"дом", L"/home/user"}) ==
          L"Alias 'дом' -> /home/user");
    // "Delete alias '{0}'?"
    CHECK(substitutePlaceholders(L"Delete alias '{0}'?", {L"работа"}) == L"Delete alias 'работа'?");
}