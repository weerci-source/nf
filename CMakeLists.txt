cmake_minimum_required(VERSION 3.31)
project(nf VERSION 1.0.0 LANGUAGES CXX)

# --- Генерация compile_commands.json для clangd ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Стандарт ---
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Отключаем сканирование C++-модулей (clang-scan-deps не нужен) ---
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# --- Куда ставить плагин (для far2l) ---
if (NOT DEFINED FAR2L_PLUGINS_DIR)
    set(FAR2L_PLUGINS_DIR "$ENV{HOME}/.local/share/far2l/Plugins"
            CACHE PATH "Directory where far2l loads plugins from")
endif ()

# --- Путь к SDK ---
set(FAR2L_SRC_DIR    "${CMAKE_SOURCE_DIR}/external/far2l")
set(FAR2L_SDK_DIR    "${FAR2L_SRC_DIR}/far2l/far2sdk")
set(FAR2L_WINPORT_DIR "${FAR2L_SRC_DIR}/WinPort")

# --- Общие флаги компиляции ---
add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
)

# --- Опции тестов ---
option(NF_BUILD_TESTS "Build tests" ON)

# --- Поддиректории ---
add_subdirectory(src)
if (NF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()