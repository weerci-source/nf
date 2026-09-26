#include <farplug-wide.h>

#include <cwchar>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include "error_logger.h"
#include "panel_data.h"
#include "paths.h"
#include "plugin.h"
#include "plugin_context.h"

namespace {

inline const PluginStartupInfo& Psi() {
    return nf::PluginContext::Instance().info();
}

std::unique_ptr<nf::Plugin> g_plugin;

wchar_t* dupW(const std::wstring& s) {
    auto* p = static_cast<wchar_t*>(malloc((s.size() + 1) * sizeof(wchar_t)));
    if (p)
        wcscpy(p, s.c_str());
    return p;
}

} // namespace

// ---------------------------------------------------------------------------
// Общие точки входа плагина
// ---------------------------------------------------------------------------

SHAREDSYMBOL int WINAPI EXP_NAME(GetMinFarVersion)() {
    return MAKEFARVERSION(2, 0);
}

SHAREDSYMBOL void WINAPI EXP_NAME(SetStartupInfo)(const struct PluginStartupInfo* Info) {
    if (!Info)
        return;

    // 1. Заполняем единый контекст — все Psi()/GetMsg() ходят сюда.
    nf::PluginContext::Init(*Info);

    // 2. Инициализируем логгер (путь — ~/.config/nf/nf.log).
    nf::ErrorLogger::init(nf::logPath(), nf::LogOutput::Both, nf::ErrorSeverity::Error);

    // 3. Теперь создаём плагин — он уже может пользоваться контекстом.
    g_plugin = std::make_unique<nf::Plugin>();
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetPluginInfo)(struct PluginInfo* Info) {
    Info->StructSize = sizeof(struct PluginInfo);
    Info->Flags = 0;          // VFS-плагин + командный префикс, без PF_DIALOG/PF_DISABLEPANELS
    Info->SysID = 0x4E46504C; // 'NFPL' — нужен уникальный ID

    // Info->GetMsg владеет строкой, указатель жив до выгрузки плагина.
    static const wchar_t* s_menu_strings[1];
    s_menu_strings[0] = nf::GetMsg(nf::MsgID::PluginTitle);
    Info->PluginMenuStrings = s_menu_strings;
    Info->PluginMenuStringsNumber = 1;

    static const wchar_t* s_config_strings[1];
    s_config_strings[0] = nf::GetMsg(nf::MsgID::PluginTitle);
    Info->PluginConfigStrings = s_config_strings;
    Info->PluginConfigStringsNumber = 1;

    Info->CommandPrefix = L"cd";
}

SHAREDSYMBOL HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item) {
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

SHAREDSYMBOL void WINAPI EXP_NAME(ExitFAR)() {
    g_plugin.reset();
}

// Обязательный экспорт, даже если настроек пока нет.
SHAREDSYMBOL int WINAPI EXP_NAME(Configure)(int /*ItemNumber*/) {
    return FALSE;
}

// ---------------------------------------------------------------------------
// Колбэки виртуальной панели
// ---------------------------------------------------------------------------

SHAREDSYMBOL int WINAPI EXP_NAME(GetFindData)(HANDLE hPlugin, PluginPanelItem** pPanelItem, int* pItemsNumber, int /*OpMode*/) {

    if (pPanelItem)
        *pPanelItem = nullptr;
    if (pItemsNumber)
        *pItemsNumber = 0;

    auto* data = reinterpret_cast<nf::PanelData*>(hPlugin);
    if (!data || data->entries.empty()) {
        // Если плагина нет, возвращаем FALSE, если он пустой — TRUE (штатная ситуация)
        return data ? TRUE : FALSE;
    }

    const auto count = data->entries.size();

    // Выделяем память через calloc, так как Far Manager API требует C-allocator
    // для последующего освобождения через free()
    auto* items = static_cast<PluginPanelItem*>(std::calloc(count, sizeof(PluginPanelItem)));
    if (!items) {
        return FALSE;
    }

    for (auto [idx, entry] : data->entries | std::ranges::views::enumerate) {
        // idx имеет правильный беззнаковый тип, соответствующий размеру контейнера
        items[idx].FindData.lpwszFileName = dupW(entry.display);
        items[idx].FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    }

    *pPanelItem = items;
    *pItemsNumber = static_cast<int>(count);
    return TRUE;
}

SHAREDSYMBOL void WINAPI EXP_NAME(FreeFindData)(HANDLE /*hPlugin*/, PluginPanelItem* pPanelItem, int pItemsNumber) {
    if (!pPanelItem || pItemsNumber <= 0) {
        return;
    }

    // C++20/23 std::span: безопасно оборачиваем сырой указатель и размер.
    // Так как Far API передает размер как знаковый int, кастим его к size_t.
    auto itemsSpan = std::span(pPanelItem, static_cast<std::size_t>(pItemsNumber));

    for (const auto& item : itemsSpan) {
        std::free(const_cast<wchar_t*>(item.FindData.lpwszFileName));
    }

    std::free(pPanelItem);
}

SHAREDSYMBOL void WINAPI EXP_NAME(GetOpenPluginInfo)(HANDLE hPlugin, OpenPluginInfo* Info) {
    auto* data = reinterpret_cast<nf::PanelData*>(hPlugin);
    Info->StructSize = sizeof(OpenPluginInfo);
    Info->Flags = 0;
    Info->HostFile = data ? data->hostFile.c_str() : L"nf-aliases";
    Info->CurDir = L"";
    Info->PanelTitle = data ? data->title.c_str() : L"nf aliases";
    Info->Format = nullptr;
}

SHAREDSYMBOL void WINAPI EXP_NAME(ClosePlugin)(HANDLE hPlugin) {
    delete reinterpret_cast<nf::PanelData*>(hPlugin);
}

SHAREDSYMBOL int WINAPI EXP_NAME(SetDirectory)(HANDLE hPlugin, const wchar_t* Dir, int /*OpMode*/) {
    auto* data = reinterpret_cast<nf::PanelData*>(hPlugin);
    if (!data) {
        return FALSE;
    }

    if (auto target = data->pathForDisplay(Dir ? Dir : L""); target && !target->empty()) {
        Psi().Control(hPlugin, FCTL_CLOSEPLUGIN, 0, 0);
        Psi().Control(PANEL_ACTIVE, FCTL_SETPANELDIR, 0, reinterpret_cast<LONG_PTR>(target->c_str()));
        return TRUE;
    }
    return FALSE;
}

SHAREDSYMBOL int WINAPI EXP_NAME(ProcessKey)(HANDLE hPlugin, int Key, unsigned int ControlState) {
    if (Key != VK_DELETE || ControlState != 0) {
        return FALSE;
    }
    auto* data = reinterpret_cast<nf::PanelData*>(hPlugin);
    if (!data) {
        return FALSE;
    }

    PanelInfo pi{};
    if (!Psi().Control(hPlugin, FCTL_GETPANELINFO, 0, reinterpret_cast<LONG_PTR>(&pi))) {
        return FALSE;
    }
    const int idx = pi.CurrentItem;
    if (idx < 0 || idx >= static_cast<int>(data->entries.size())) {
        return FALSE;
    }

    const auto name = data->nameAt(static_cast<std::size_t>(idx));
    if (!name) {
        return FALSE;
    }
    if (!g_plugin) {
        return TRUE;
    }

    const std::wstring title = nf::GetMsg(nf::MsgID::DeleteAliasTitle);
    const std::wstring body = nf::FormatMsg(nf::MsgID::DeleteAliasQuestion, {*name});
    const wchar_t* items[] = {title.c_str(), body.c_str()};
    const int r = Psi().Message(Psi().ModuleNumber, FMSG_MB_YESNO, nullptr, items, 2, 0);
    if (r != 0) {
        return TRUE;
    }

    if (auto res = g_plugin->aliases().remove(*name); !res) {
        nf::ErrorLogger::log(res.error());
        return TRUE;
    }

    data->eraseAt(static_cast<std::size_t>(idx));
    Psi().Control(hPlugin, FCTL_UPDATEPANEL, 0, 0);
    Psi().Control(hPlugin, FCTL_REDRAWPANEL, 0, 0);
    return TRUE;
}