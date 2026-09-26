// Компиляционный smoke-тест тулчейна.
//
// Не проверяет логику nf — проверяет, что компилятор и стандартная
// библиотека поддерживают фичи C++23, на которые опирается код плагина:
//   - std::ranges::to<std::vector>   (P1206)
//   - std::views::enumerate          (P2164)
//   - std::wstring::contains         (P1679)
//   - __cplusplus >= 202302L
//
// Если этот файл не компилируется — значит либо компилятор старее
// Clang 17 / GCC 14, либо libstdc++ старее GCC 14, либо
// CMAKE_CXX_STANDARD понижен ниже 23. Все три случая делают сборку
// плагина бессмысленной, поэтому падаем рано и с понятным сообщением.
#include <ranges>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("toolchain: __cplusplus указывает на C++23", "[toolchain]") {
    static_assert(__cplusplus >= 202302L,
                  "nf требует C++23. Проверьте CMAKE_CXX_STANDARD и версию компилятора.");
    SUCCEED();
}

TEST_CASE("toolchain: std::ranges::to<std::vector> (P1206)", "[toolchain]") {
    std::vector<int> v{1, 2, 3};
    auto w = v | std::ranges::to<std::vector>();
    REQUIRE(w == v);
}

TEST_CASE("toolchain: std::views::enumerate (P2164)", "[toolchain]") {
    std::vector<int> v{10, 20, 30};
    int sum = 0;
    for (auto [i, x] : v | std::views::enumerate) {
        sum += static_cast<int>(i) + x;
    }
    // (0+10) + (1+20) + (2+30) = 63
    REQUIRE(sum == 63);
}

TEST_CASE("toolchain: std::wstring::contains (P1679)", "[toolchain]") {
    const std::wstring s = L"nf — directory aliases";
    REQUIRE(s.contains(L"directory"));
    REQUIRE_FALSE(s.contains(L"nope"));
}