find_package(Catch2 3 REQUIRED)

add_executable(nf_tests
        test_plugin.cpp
)

target_include_directories(nf_tests PRIVATE
        "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(nf_tests PRIVATE
        Catch2::Catch2WithMain
)

# Регистрируем каждый TEST_CASE как отдельный тест в CTest
include(Catch)
catch_discover_tests(nf_tests)