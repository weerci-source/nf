# Разработка плагина для far2l

Документ описывает, как far2l загружает плагин, какие символы он ищет,
как устроен жизненный цикл, и как писать каждую функцию.

## 1. Что такое плагин far2l

Плагин — динамически загружаемая библиотека (`.so` в Linux), которая
экспортирует определённый набор функций. far2l загружает её через `dlopen`,
проверяет наличие обязательных символов, вызывает их в строгом порядке
и передаёт плагину таблицу функций API через `PluginStartupInfo`.

Имя файла и путь важны:

```
<Plugins>/<name>/plug/<name>.far-plug-wide
```

- `.far-plug-wide` — плагин под wide API (UTF-16 строки). Современный вариант.
- `.far-plug-mb` — legacy multibyte API. Не используйте для новых плагинов.

Стандартные директории поиска:

| Директория | Назначение |
|---|---|
| `/usr/lib/far2l/Plugins/` | системные плагины из пакета |
| `~/.local/share/far2l/Plugins/` | пользовательские (в некоторых сборках) |

В каждой поддиректории far2l ищет файл `plug/*.far-plug-wide`.

## 2. Обязательный минимум

Каждый плагин экспортирует три функции:

| Функция | Возврат | Когда вызывается |
|---|---|---|
| `GetMinFarVersionW` | `int` | сразу после загрузки `.so` |
| `SetStartupInfoW` | `void` | после проверки версии |
| `GetPluginInfoW` | `void` | сразу после `SetStartupInfoW` |

Если плагин открывает собственную панель — добавляются:

| Функция | Возврат | Когда вызывается |
|---|---|---|
| `OpenPluginW` | `HANDLE` | открытие панели (меню, префикс, макрос) |
| `ClosePluginW` | `void` | закрытие панели |
| `GetOpenPluginInfoW` | `void` | запрос метаданных панели |
| `GetFindDataW` | `int` | получение списка элементов панели |
| `FreeFindDataW` | `void` | освобождение памяти после `GetFindDataW` |
| `SetDirectoryW` | `int` | Enter на элементе (навигация) |

Опционально: `GetFilesW`, `PutFilesW`, `DeleteFilesW`, `MakeDirectoryW`,
`ProcessKeyW`, `ProcessEventW`, `ProcessEditorEventW`, `ProcessViewerEventW`,
`ProcessDialogEventW`, `ProcessSynchroEventW`, `CompareW`,
`GetLinkTargetW`, `ConfigureW`, `ExitFARW`, `MayExitFARW`,
`OpenFilePluginW`, `AnalyseW`, `CloseAnalyseW`.

Полный список — в `farplug-wide.h`, секция `Exported Functions`.

## 3. Жизненный цикл плагина

```
far2l стартует
   ↓
dlopen(".../plug/nf.far-plug-wide")
   ↓
dlsym(GetMinFarVersionW) ──→ int  (сравнивается с FARMANAGERVERSION)
   ↓
dlsym(SetStartupInfoW)  ──→ void (передаётся PluginStartupInfo)
   ↓
dlsym(GetPluginInfoW)   ──→ void (флаги, меню, префикс)
   ↓
плагин зарегистрирован (виден в F11 и по префиксу команды)
   ↓
пользователь открывает плагин
   ↓
OpenPluginW(OpenFrom, Item)
   ├── вернуть INVALID_HANDLE_VALUE — плагин «выполнил действие и закрылся»
   └── вернуть валидный HANDLE     — открыта панель
         ↓
      GetOpenPluginInfoW  (метаданные панели)
      GetFindDataW        (список элементов)
         ↓
      [пользователь работает с панелью]
         ├── SetDirectoryW   при Enter на элементе
         ├── GetFilesW / PutFilesW / DeleteFilesW — по F5/F6/F8
         └── ProcessKeyW     при нажатии клавиш в панели
         ↓
      FreeFindDataW  (освободить память)
      ClosePluginW   (закрыть панель)
```

## 4. Описание каждой экспортируемой функции

### GetMinFarVersionW

```cpp
int WINAPI GetMinFarVersionW() {
    return MAKEFARVERSION(2, 0);
}
```

- **Когда:** сразу после `dlopen`.
- **Зачем:** сообщить минимальную версию far2l, с которой совместим плагин.
- **Что вернуть:** `MAKEFARVERSION(major, minor)`. В текущем far2l макрос
  принимает ровно два аргумента. `MAKEFARVERSION(2, 0)` покрывает
  актуальные сборки far2l.

### SetStartupInfoW

```cpp
void WINAPI SetStartupInfoW(const PluginStartupInfo* Info) {
    if (!Info) return;
    g_psi = *Info;
    // ...
}
```

- **Когда:** после проверки версии, до `GetPluginInfoW`.
- **Зачем:** far2l передаёт плагину таблицу функций API.
- **Что сохранить:**
  - `psi_.ModuleNumber` — идентификатор модуля, нужен в вызовах `Menu`,
    `Message`, `Control` и т.д.
  - `psi_.FSF` — указатель на `FarStandardFunctions` (работа со строками,
    путями, меню, поиском).
  - `psi_.Message` — вывод диалога.
  - `psi_.Menu` — вывод меню.
  - `psi_.Control` — управление панелями.
  - `psi_.AdvControl` — расширенные операции (обновление, выход).

Сохраняйте копию `*Info` в глобальной переменной — сам указатель
принадлежит far2l и может стать невалидным.

### GetPluginInfoW

```cpp
void WINAPI GetPluginInfoW(PluginInfo* Info) {
    Info->StructSize = sizeof(*Info);
    Info->Flags      = PF_EDITOR | PF_VIEWER | PF_DIALOG;
    Info->PluginMenuStrings       = kPluginMenuStrings;
    Info->PluginMenuStringsNumber = 1;
    Info->CommandPrefix           = L"nf";
}
```

- **Когда:** после `SetStartupInfoW`.
- **Зачем:** сообщить far2l, как отображать плагин и как его вызывать.

Флаги (`PLUGIN_FLAGS`):

| Флаг | Значение |
|---|---|
| `PF_EDITOR` | реагирует на события редактора (`ProcessEditorEventW`, `ProcessEditorInputW`) |
| `PF_VIEWER` | реагирует на события просмотрщика |
| `PF_DIALOG` | реагирует на события диалогов (`ProcessDialogEventW`) |
| `PF_FULLCMDLINE` | получает всю командную строку через `OpenPluginW`, а не остаток после префикса |
| `PF_PRELOAD` | загрузить плагин при старте far2l |
| `PF_PREOPEN` | загрузить библиотеку сразу, инициализировать позже |
| `PF_DISABLEPANELS` | запретить открывать панели |

Меню (`PluginMenuStrings` / `PluginMenuStringsNumber`) — массив wide-строк,
которые появятся в **F11** (меню плагинов). Число должно совпадать.

`CommandPrefix` — префикс для вызова из командной строки. Например, `L"nf"` —
пользователь может ввести `nf:something` в командной строке, far2l
вызовет `OpenPluginW(OPEN_COMMANDLINE, ...)`.

### OpenPluginW

```cpp
HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item) {
    // ...
}
```

- **Когда:** пользователь выбрал пункт меню, ввёл команду с префиксом,
  вызвал плагин из макроса.
- **Параметры:**
  - `OpenFrom` — откуда открыт плагин. Возможные значения:
    `OPEN_DISKMENU`, `OPEN_PLUGINSMENU`, `OPEN_FINDLIST`, `OPEN_SHORTCUT`,
    `OPEN_COMMANDLINE`, `OPEN_EDITOR`, `OPEN_VIEWER`, `OPEN_FILEPANEL`,
    `OPEN_DIALOG`, `OPEN_ANALYSE`, `OPEN_FROMMACRO`, `OPEN_FROMMACROSTRING`.
  - `Item` — зависит от `OpenFrom`. Для `OPEN_COMMANDLINE` — указатель на
    `wchar_t*` с остатком строки после префикса. Для `OPEN_PLUGINSMENU` —
    индекс выбранного пункта меню.
- **Возвращает:**
  - `INVALID_HANDLE_VALUE` — плагин не открывает панель (например,
    выполнил действие и завершился).
  - валидный `HANDLE` — открыта панель. far2l будет вызывать панельные
    функции с этим `HANDLE`.

Часто в качестве `HANDLE` возвращают указатель на свою структуру
(`static_cast<HANDLE>(panel_data)`). far2l его не интерпретирует, просто
передаёт обратно в панельные функции.

### ClosePluginW

```cpp
void WINAPI ClosePluginW(HANDLE hPlugin) {
    auto* data = static_cast<PanelData*>(hPlugin);
    delete data;
}
```

- **Когда:** панель закрывается пользователем, программой или самим плагином.
- **Задача:** освободить всё, что было выделено в `OpenPluginW`.

### GetOpenPluginInfoW

```cpp
void WINAPI GetOpenPluginInfoW(HANDLE hPlugin, OpenPluginInfo* Info) {
    Info->StructSize = sizeof(*Info);
    Info->PanelTitle = L"nf";
    // ...
}
```

- **Когда:** сразу после `OpenPluginW` и при каждой перерисовке панели.
- **Задача:** заполнить метаданные панели:
  - `PanelTitle` — заголовок.
  - `CurDir` — «виртуальный» текущий каталог.
  - `Format` — формат панели.
  - `InfoLines` — строки-подсказки под панелью.
  - `KeyBar` — переопределить клавиши в нижней строке.
  - `Flags` — `OPIF_REALNAMES`, `OPIF_SHOWNAMESONLY` и др.

### GetFindDataW

```cpp
int WINAPI GetFindDataW(HANDLE hPlugin,
                        PluginPanelItem** pPanelItem,
                        int* pItemsNumber,
                        int OpMode) {
    // ...
}
```

- **Когда:** far2l нужен список элементов панели.
- **Задача:** выделить массив `PluginPanelItem` и заполнить его.

Каждый элемент:

```cpp
PluginPanelItem item{};
item.FindData.lpwszFileName = L"example";   // имя
item.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY; // как папка
item.Description = L"пояснение";            // колонка описания
```

Память выделяет плагин. far2l потом вызовет `FreeFindDataW` — там же и
освободите.

### FreeFindDataW

```cpp
void WINAPI FreeFindDataW(HANDLE hPlugin,
                          PluginPanelItem* pPanelItem,
                          int pItemsNumber) {
    // ...
}
```

- **Когда:** far2l закончил с данными.
- **Задача:** пройти по массиву и освободить каждую строку, затем массив.
  Удобно использовать `psi_.FSF->DeleteBuffer` или `free` — что
  использовали при выделении.

### SetDirectoryW

```cpp
int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t* Dir, int OpMode) {
    // ...
}
```

- **Когда:** пользователь нажал Enter на элементе панели.
- **Задача:** обработать «вход» в элемент.
  - Для панели-каталога: перейти в подкаталог.
  - Для виртуальной панели: выполнить произвольное действие.
- **Возврат:** `TRUE` — действие выполнено, `FALSE` — far2l покажет
  стандартное поведение.

Часто внутри вызывают `psi_.Control(PANEL_ACTIVE, FCTL_SETPANELDIR, 0, (LONG_PTR)path)`
и `FCTL_UPDATEPANEL`, чтобы заставить far2l перерисовать панель на новом
каталоге. После успеха панель плагина можно закрыть:
`psi_.Control(hPlugin, FCTL_CLOSEPLUGIN, 0, 0)`.

## 5. Работа с API far2l

### Показ сообщения

```cpp
const wchar_t* items[] = { L"Заголовок", L"Текст сообщения" };
psi_.Message(psi_.ModuleNumber, FMSG_MB_OK, nullptr, items, 2, 0);
```

Первая строка — заголовок, остальные — тело. Флаги кнопок:
`FMSG_MB_OK`, `FMSG_MB_OKCANCEL`, `FMSG_MB_YESNO`, `FMSG_MB_YESNOCANCEL`,
`FMSG_MB_ABORTRETRYIGNORE`, `FMSG_MB_RETRYCANCEL`. Модификаторы:
`FMSG_WARNING`, `FMSG_ERRORTYPE`, `FMSG_LEFTALIGN`.

### Меню

```cpp
FarMenuItem items[] = {
    { L"Say Hello", 0, 0, 0 },
    { L"Exit",      0, 0, 0 },
};

int choice = psi_.Menu(psi_.ModuleNumber,
                       -1, -1, 0,
                       FMENU_WRAPMODE,
                       L"nf plugin", L"Choose an action", L"nf",
                       nullptr, nullptr,
                       items, 2);
```

`FarMenuItem`:

```cpp
struct FarMenuItem {
    const wchar_t* Text;
    int Selected;
    int Checked;
    int Separator;
};
```

Флаги (`FARMENUFLAGS`): `FMENU_WRAPMODE`, `FMENU_AUTOHIGHLIGHT`,
`FMENU_REVERSEAUTOHIGHLIGHT`, `FMENU_SHOWAMPERSAND`, `FMENU_USEEXT`,
`FMENU_CHANGECONSOLETITLE`.

### Управление панелями

```cpp
psi_.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0, (LONG_PTR)buf);
psi_.Control(PANEL_ACTIVE, FCTL_SETPANELDIR, 0, (LONG_PTR)new_dir);
psi_.Control(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
psi_.Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
psi_.Control(hPlugin, FCTL_CLOSEPLUGIN, 0, 0);
```

Основные команды `FILE_CONTROL_COMMANDS` описаны в `farplug-wide.h`.

### FSF — стандартные функции

```cpp
psi_.FSF->PointToName(path);         // отбросить путь, оставить имя файла
psi_.FSF->GetPathRoot(path, root, size);
psi_.FSF->LStricmp(a, b);            // регистронезависимое сравнение
psi_.FSF->TruncStr(str, max_cells);  // обрезка по числу колонок
psi_.FSF->MkTemp(buf, size, L"nf_"); // создать временный файл
```

Полный список полей — `struct FarStandardFunctions` в `farplug-wide.h`.

## 6. Отладка

- Соберите Debug-версию: `cmake --build build`.
- Скопируйте `.far-plug-wide` в директорию плагинов far2l.
- В VS Code выберите конфигурацию **nf: Debug in far2l (lldb)** и нажмите F5.
- Точки останова в `src/*.cpp` будут работать.

Так как far2l запускается как GUI-приложение, отладчик цепляется к процессу
`/usr/bin/far2l`. Плагин загружается в момент старта — поэтому, если
нужна отладка с самого начала, поставьте breakpoint в `SetStartupInfoW`.

### Логирование

Вместо отладчика удобно логировать в файл:

```cpp
#include <cstdio>
static void Log(const char* fmt, ...) {
    FILE* f = fopen("/tmp/nf.log", "a");
    if (!f) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fputc('\n', f);
    fclose(f);
}
```

Для wide-строк используйте `%ls`.

## 7. Типичные грабли

- **Плагин не появляется в F11.** Проверьте:
  - имя файла заканчивается на `.far-plug-wide`;
  - путь правильный: `<Plugins>/<name>/plug/<name>.far-plug-wide`;
  - экспортируются `GetMinFarVersionW`, `SetStartupInfoW`, `GetPluginInfoW`
    (`nm -D --defined-only <file>.far-plug-wide` покажет `T` рядом с ними);
  - нет ошибок в `ldd` (`ldd <file>.far-plug-wide`).
- **`GetMinFarVersionW` объявлена как `DWORD`, а должна быть `int`.**
  Загляните в `farplug-wide.h` — там точная сигнатура.
- **`MAKEFARVERSION` требует двух аргументов**, а не пяти. В старых
  примерах — `MAKEFARVERSION(1, 0, 0, 0, VS_RELEASE)`, в актуальном
  far2l — `MAKEFARVERSION(2, 0)`.
- **Структура `PluginInfo` не содержит GUID'ов.** Это API FAR 2.x, не
  FAR 3.x. Меню — через `PluginMenuStrings`, а не через `MenuString`.
- **Копирование в `/usr/lib/far2l/Plugins/` требует root.** Либо
  `sudo cp`, либо пользовательская директория плагинов (если far2l
  её сканирует — проверьте в вашей сборке).

## 8. Полезные ссылки

- Репозиторий far2l: <https://github.com/elfmz/far2l>
- Директория `plugins/` в far2l — примеры плагинов, включая `python`,
  `NetRocks`, `colorer`, `tmppanel`. Читать их исходники — лучший способ
  разобраться в API.
- `farplug-wide.h` — единственный достоверный источник по API.