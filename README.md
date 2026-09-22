# far2l plugin template

Шаблон C++ проекта для разработки плагинов к [far2l](https://github.com/elfmz/far2l)
на Clang + CMake + Ninja, с юнит-тестами на Catch2 и готовой интеграцией в VS Code.

Проект собирается как `.far-plug-wide` (широкие символы, современный API far2l)
и автоматически ставится в директорию плагинов far2l.

## Что внутри

- **Clang** как компилятор (и `clangd` как LSP в VS Code).
- **CMake + Ninja** для сборки, `compile_commands.json` для `clangd`.
- **Catch2 v3** для тестов, интеграция с CTest через `catch_discover_tests`.
- **VS Code** задачи и конфиги для сборки, тестирования и запуска far2l.
- **Пример плагина `nf`** — минимальный, с меню и диалогом; служит отправной точкой.
- **Шаблоны панельного плагина** (закомментированы в `src/main.cpp`) —
  `OpenPlugin`, `GetFindData`, `FreeFindData`, `SetDirectory` и т.д.
- Подробная документация по API: [`doc/DEVELOPMENT.md`](doc/DEVELOPMENT.md).

## Требования

- Linux (тестировалось на Debian/Ubuntu-подобных).
- `clang` ≥ 16 (проверено на 19), `cmake` ≥ 3.28, `ninja`, `lldb` или `gdb`.
- Установленный `far2l` (TTY или GUI — не важно).
- Пакет **far2l SDK**: заголовки `farplug-wide.h`, `farcommon.h` и др. из репозитория far2l.
- Для тестов: `libcatch2-dev` (Catch2 v3).
- Опционально: VS Code + расширения
  `llvm-vs-code-extensions.vscode-clangd`, `ms-vscode.cmake-tools`.

Установка на Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y clang cmake ninja-build lldb gdb catch2 libcatch2-dev
```

## Получение far2l SDK

```bash
cd external
git clone --depth 1 https://github.com/elfmz/far2l.git
```

В итоге ожидается структура:

```
external/far2l/               <- корень репозитория far2l
├── far2l/
│   └── far2sdk/
│       └── farplug-wide.h
├── WinPort/
│   └── windows.h
└── ...
```

CMake сам найдёт `far2l/far2sdk` и `WinPort` по относительным путям.

## Сборка

### Debug

```bash
cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

### Release

```bash
cmake -S . -B build-release -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DNF_BUILD_TESTS=OFF \
      -DCMAKE_CXX_COMPILER=clang++
cmake --build build-release
```

## Тесты

```bash
cd build && ctest --output-on-failure
```

Или напрямую — с выводом Catch2:

```bash
./build/tests/nf_tests
./build/tests/nf_tests "[nf]"
./build/tests/nf_tests --list-tests
```

## Установка плагина в far2l

far2l ищет плагины в подкаталогах вида `<Plugins>/<name>/plug/<name>.far-plug-wide`.

Например, для системной установки:

```bash
sudo mkdir -p /usr/lib/far2l/Plugins/nf/plug
sudo cp build-release/src/nf.far-plug-wide /usr/lib/far2l/Plugins/nf/plug/
```

Или через задачу VS Code `nf: install (Release)`.

Перезапустите far2l, нажмите **F11** — плагин `nf` должен появиться в списке.

## Работа в VS Code

Установите расширения:

- `llvm-vs-code-extensions.vscode-clangd` — автодополнение, переходы, диагностика.
- `ms-vscode.cmake-tools` — интеграция с CMake и CTest.

Отключите встроенный C/C++ от Microsoft (если стоит) — чтобы не конфликтовал с `clangd`.

Готовые задачи (**Ctrl+Shift+P → Tasks: Run Task**):

| Задача | Что делает |
|---|---|
| `nf: build (Debug)` | Конфигурирует и собирает Debug. |
| `nf: build (Debug) + test` | То же + CTest. |
| `nf: build (Release)` | Конфигурирует и собирает Release. |
| `nf: install (Release)` | Копирует Release-плагин в `/usr/lib/far2l/Plugins/nf/plug/`. |
| `nf: build (Release) + install` | Всё сразу: собрать Release и поставить. |
| `nf: run far2l` | Собирает Debug и запускает far2l. |

Отладка в `launch.json`: `nf: Debug in far2l (lldb)` и `nf: Debug tests (lldb)`.

## Использование шаблона как точки старта нового плагина

1. Скопируйте директорию проекта под новым именем.
2. Переименуйте плагин:
   - в `CMakeLists.txt` — `project(<newname> ...)`;
   - в `src/CMakeLists.txt` — `add_library(<newname> ...)`,
     `set_target_properties(<newname> PROPERTIES OUTPUT_NAME "<newname>" ...)`,
     `set(NF_PLUGIN_DIR ...)` → `<newname>/plug`;
   - в `src/main.cpp` — строки меню, `CommandPrefix`, `ModuleName`;
   - в `tests/CMakeLists.txt` — имя тестового таргета.
3. Обновите `include/<newname>/Plugin.h` — заголовок, namespace.
4. Пересоберите:

   ```bash
   rm -rf build build-release
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
   cmake --build build
   ```

## Документация

- [`doc/DEVELOPMENT.md`](doc/DEVELOPMENT.md) — жизненный цикл плагина,
  полный список экспортируемых функций, шаблоны кода, взаимодействие с far2l.

## Лицензия

Шаблон распространяется как есть. Код far2l SDK — под лицензией far2l.
