#include <farplug-wide.h>

#include <cwchar>
#include <memory>
#include <string>
#include <vector>

#include "error_logger.h" // ← добавить в начало main.cpp
#include "panel_data.h"
#include "paths.h" // ← для logPath()
#include "plugin.h"
#include "plugin_context.h" // ← для PluginContext::Init

namespace {

inline const PluginStartupInfo& Psi() {
    return nf::PluginContext::Instance().info();
}

std::unique_ptr<nf::Plugin> g_plugin;

const wchar_t* kPluginMenuStrings[] = {L"nf (directory aliases)"};

wchar_t* dupW(const std::wstring& s) {
    auto* p = static_cast<wchar_t*>(malloc((s.size() + 1) * sizeof(wchar_t)));
    if (p)
        wcscpy(p, s.c_str());
    return p;
}

} // namespace

extern "C" {

// ---------------------------------------------------------------------------
// Общие точки входа плагина
// ---------------------------------------------------------------------------

int WINAPI GetMinFarVersionW() {
    return MAKEFARVERSION(2, 0);
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo* Info) {
    if (!Info)
        return;

    // 1. Заполняем единый контекст — все Psi()/GetMsg() ходят сюда.
    nf::PluginContext::Init(*Info);

    // 2. Инициализируем логгер (путь — ~/.config/nf/nf.log).
    nf::ErrorLogger::init(nf::logPath(), nf::LogOutput::Both, nf::ErrorSeverity::Error);

    // 3. Теперь создаём плагин — он уже может пользоваться контекстом.
    g_plugin = std::make_unique<nf::Plugin>();
}

void WINAPI GetPluginInfoW(struct PluginInfo* Info) {
    Info->StructSize = sizeof(struct PluginInfo);
    Info->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG;

    Info->PluginMenuStrings = kPluginMenuStrings;
    Info->PluginMenuStringsNumber = 1;

    // Префикс "cd" в командной строке -> OpenPluginW(OPEN_COMMANDLINE)
    Info->CommandPrefix = L"cd";
}

HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item) {
    if (!g_plugin)
        return INVALID_HANDLE_VALUE;

    if (OpenFrom == OPEN_FROMMACROSTRING || OpenFrom == OPEN_COMMANDLINE) {
        const wchar_t* rest = Item ? reinterpret_cast<const wchar_t*>(Item) : L"";
        return g_plugin->HandleCommand(rest);
    }

    if (OpenFrom == OPEN_PLUGINSMENU) {
        return g_plugin->HandleCommand(L"");
    }

    return INVALID_HANDLE_VALUE;
}

void WINAPI ExitFARW() {
    g_plugin.reset();
}

// ---------------------------------------------------------------------------
// Колбэки виртуальной панели
// ---------------------------------------------------------------------------

int WINAPI GetFindDataW(HANDLE hPlugin, PluginPanelItem** pPanelItem, int* pItemsNumber,
                        int /*OpMode*/) {
    auto* data = reinterpret_cast<nf::PanelData*>(hPlugin);
    if (!data) {
        *pPanelItem = nullptr;
        *pItemsNumber = 0;
        return FALSE;
    }

    const int n = static_cast<int>(data->names.size());
    if (n == 0) {
        *pPanelItem = nullptr;
        *pItemsNumber = 0;
        return TRUE;
    }

    auto* items = static_cast<PluginPanelItem*>(calloc(n, sizeof(PluginPanelItem)));
    if (!items) {
        *pPanelItem = nullptr;
        *pItemsNumber = 0;
        return FALSE;
    }

    for (int i = 0; i < n; ++i) {
        const std::wstring& display = (static_cast<std::size_t>(i) < data->displayNames.size())
                                          ? data->displayNames[i]
                                          : data->names[i];
        items[i].FindData.lpwszFileName = dupW(display);
        items[i].FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    }

    *pPanelItem = items;
    *pItemsNumber = n;
    return TRUE;
}

void WINAPI FreeFindDataW(HANDLE /*hPlugin*/, PluginPanelItem* pPanelItem, int pItemsNumber) {
    for (int i = 0; i < pItemsNumber; ++i) {
        free(const_cast<wchar_t*>(pPanelItem[i].FindData.lpwszFileName));
    }
    free(pPanelItem);
}

void WINAPI GetOpenPluginInfoW(HANDLE hPlugin, OpenPluginInfo* Info) {
    auto* data = reinterpret_cast<nf::PanelData*>(hPlugin);

    Info->StructSize = sizeof(OpenPluginInfo);
    Info->Flags = 0;
    Info->HostFile = data ? data->hostFile.c_str() : L"nf-aliases";
    Info->CurDir = L"";
    Info->PanelTitle = data ? data->title.c_str() : L"nf aliases";
    Info->Format = L"";
}

void WINAPI ClosePluginW(HANDLE hPlugin) {
    delete reinterpret_cast<nf::PanelData*>(hPlugin);
}

int WINAPI SetDirectoryW(HANDLE hPlugin, const wchar_t* Dir, int /*OpMode*/) {
    auto* data = reinterpret_cast<nf::PanelData*>(hPlugin);
    if (!data)
        return FALSE;

    std::wstring target;
    for (size_t i = 0; i < data->displayNames.size(); ++i) {
        if (data->displayNames[i] == Dir) {
            target = data->paths[i];
            break;
        }
    }
    // Защита: если far2l передал чистое имя (например, в старых версиях) —
    // сравниваем и с names.
    if (target.empty()) {
        for (size_t i = 0; i < data->names.size(); ++i) {
            if (data->names[i] == Dir) {
                target = data->paths[i];
                break;
            }
        }
    }
    if (target.empty())
        return FALSE;

    Psi().Control(hPlugin, FCTL_CLOSEPLUGIN, 0, 0);
    Psi().Control(PANEL_ACTIVE, FCTL_SETPANELDIR, 0, reinterpret_cast<LONG_PTR>(target.c_str()));
    return TRUE;
}

int WINAPI ProcessKeyW(HANDLE hPlugin, int Key, unsigned int ControlState) {
    if (Key != VK_DELETE || ControlState != 0)
        return FALSE;
    auto* data = reinterpret_cast<nf::PanelData*>(hPlugin);
    if (!data)
        return FALSE;

    PanelInfo pi{};
    if (!Psi().Control(hPlugin, FCTL_GETPANELINFO, 0, reinterpret_cast<LONG_PTR>(&pi)))
        return FALSE;
    const int idx = pi.CurrentItem;
    if (idx < 0 || idx >= static_cast<int>(data->names.size()))
        return FALSE;

    const std::wstring name = data->names[idx];

    if (!g_plugin)
        return TRUE;

    const std::wstring title = nf::GetMsg(nf::MsgID::DeleteAliasTitle);
    const std::wstring body = nf::FormatMsg(nf::MsgID::DeleteAliasQuestion, {name});
    const wchar_t* items[] = {title.c_str(), body.c_str()};
    const int r = Psi().Message(Psi().ModuleNumber, FMSG_MB_YESNO, nullptr, items, 2, 0);

    if (r != 0)
        return TRUE; // No / Cancel

    if (auto res = g_plugin->aliases().remove(name); !res) {
        nf::ErrorLogger::log(res.error());
        return TRUE;
    }

    data->names.erase(data->names.begin() + idx);
    data->paths.erase(data->paths.begin() + idx);
    Psi().Control(hPlugin, FCTL_UPDATEPANEL, 0, 0);
    Psi().Control(hPlugin, FCTL_REDRAWPANEL, 0, 0);
    return TRUE;
}

} // extern "C"