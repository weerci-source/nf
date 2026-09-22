#include <farplug-wide.h>

#include <memory>

#include "nf/Plugin.h"

namespace {

PluginStartupInfo g_psi{};
std::unique_ptr<nf::Plugin> g_plugin;

const wchar_t* kPluginMenuStrings[] = { L"nf" };

} // namespace

extern "C" {

int WINAPI GetMinFarVersionW() {
    return MAKEFARVERSION(2, 0);
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo* Info) {
    if (!Info) {
        return;
    }
    g_psi = *Info;
    g_plugin = std::make_unique<nf::Plugin>(g_psi);
}

void WINAPI GetPluginInfoW(struct PluginInfo* Info) {
    Info->StructSize = sizeof(struct PluginInfo);
    Info->Flags      = PF_EDITOR | PF_VIEWER | PF_DIALOG;

    Info->PluginMenuStrings       = kPluginMenuStrings;
    Info->PluginMenuStringsNumber = 1;

    Info->CommandPrefix = L"nf";
}

HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR /*Item*/) {
    if (!g_plugin) {
        return INVALID_HANDLE_VALUE;
    }
    if (OpenFrom == OPEN_FROMMACRO || OpenFrom == OPEN_FROMMACROSTRING) {
        return INVALID_HANDLE_VALUE;
    }
    g_plugin->ShowMenu();
    return INVALID_HANDLE_VALUE;
}

void WINAPI ExitFARW() {
    g_plugin.reset();
}

// ---------------------------------------------------------------------------
// ПРИМЕР ПАНЕЛЬНОГО ПЛАГИНА
// ---------------------------------------------------------------------------
// Если ваш плагин открывает собственную панель (вместо ShowMenu),
// реализуйте функции ниже. Удалите/раскомментируйте по мере надобности.
//
// В качестве HANDLE удобно возвращать указатель на вашу структуру PanelData:
//
//   struct PanelData {
//       std::vector<std::wstring> items;
//       std::wstring title;
//   };
//
// far2l не интерпретирует HANDLE — просто передаёт его обратно.
// ---------------------------------------------------------------------------
//
// HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item) {
//     if (OpenFrom == OPEN_COMMANDLINE && Item) {
//         // rest — то, что пользователь ввёл после "nf:"
//         const wchar_t* rest = reinterpret_cast<const wchar_t*>(Item);
//         (void)rest;
//     }
//
//     auto* data = new PanelData{};
//     data->title = L"nf panel";
//     data->items = { L"item 1", L"item 2" };
//     return reinterpret_cast<HANDLE>(data);
// }
//
// void WINAPI ClosePluginW(HANDLE hPlugin) {
//     delete reinterpret_cast<PanelData*>(hPlugin);
// }
//
// void WINAPI GetOpenPluginInfoW(HANDLE /*hPlugin*/, struct OpenPluginInfo* Info) {
//     Info->StructSize = sizeof(struct OpenPluginInfo);
//     Info->PanelTitle = L"nf panel";
//     Info->Flags      = OPIF_REALNAMES;
// }
//
// int WINAPI GetFindDataW(HANDLE hPlugin,
//                         struct PluginPanelItem** pPanelItem,
//                         int* pItemsNumber,
//                         int /*OpMode*/) {
//     auto* data = reinterpret_cast<PanelData*>(hPlugin);
//     const int n = static_cast<int>(data->items.size());
//
//     auto* items = static_cast<PluginPanelItem*>(
//         calloc(n, sizeof(PluginPanelItem)));
//     if (!items) {
//         *pPanelItem = nullptr;
//         *pItemsNumber = 0;
//         return FALSE;
//     }
//
//     for (int i = 0; i < n; ++i) {
//         items[i].FindData.lpwszFileName = wcsdup(data->items[i].c_str());
//         items[i].FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
//     }
//
//     *pPanelItem  = items;
//     *pItemsNumber = n;
//     return TRUE;
// }
//
// void WINAPI FreeFindDataW(HANDLE /*hPlugin*/,
//                           struct PluginPanelItem* pPanelItem,
//                           int pItemsNumber) {
//     for (int i = 0; i < pItemsNumber; ++i) {
//         free(const_cast<wchar_t*>(pPanelItem[i].FindData.lpwszFileName));
//     }
//     free(pPanelItem);
// }
//
// int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t* Dir, int /*OpMode*/) {
//     auto* data = reinterpret_cast<PanelData*>(hPlugin);
//     (void)data;
//
//     // Пример: печатаем имя и закрываем панель
//     const wchar_t* items[] = { L"nf", Dir };
//     g_psi.Message(g_psi.ModuleNumber, FMSG_MB_OK, nullptr, items, 2, 0);
//     g_psi.Control(hPlugin, FCTL_CLOSEPLUGIN, 0, 0);
//     return TRUE;
// }

} // extern "C"