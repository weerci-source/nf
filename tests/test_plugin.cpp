#include <catch2/catch_test_macros.hpp>

// main() даёт Catch2::Catch2WithMain — свой писать не нужно.
// Здесь только тесты.

TEST_CASE("sanity: арифметика работает", "[sanity]") {
    CHECK(1 + 1 == 2);
}

TEST_CASE("nf: имя плагина", "[nf]") {
    const std::string name = "nf";
    REQUIRE(name == "nf");
    CHECK(name.size() == 2);
}

TEST_CASE("nf: заглушка для будущей логики", "[nf][wip]") {
    // Здесь появятся тесты логики nf_core, когда вынесем её
    SUCCEED("placeholder");
}