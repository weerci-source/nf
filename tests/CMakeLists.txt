find_package(Catch2 3 REQUIRED)

set(NF_TEST_SOURCES
        test_common.cpp
        test_alias_manager.cpp
        test_stubs.cpp
        # Исходники плагина, не зависящие от far2l SDK
        ${CMAKE_SOURCE_DIR}/src/common.cpp
        ${CMAKE_SOURCE_DIR}/src/alias_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/error.cpp
        ${CMAKE_SOURCE_DIR}/src/paths.cpp
)

add_executable(nf_tests
        ${NF_TEST_SOURCES}
)

target_include_directories(nf_tests PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
)

target_compile_options(nf_tests PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
)

target_link_libraries(nf_tests PRIVATE
        Catch2::Catch2WithMain
)

# Регистрируем каждый TEST_CASE как отдельный тест в CTest
include(Catch)
catch_discover_tests(nf_tests)