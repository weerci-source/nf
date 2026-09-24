#include "plugin.h"

#include <cstdlib>
#include <filesystem>

#include "error_logger.h"
#include "panel_data.h"
#include "paths.h"
#include "plugin_context.h"

namespace nf {

namespace {

constexpr std::size_t kPanelDirBufSize = 4096;

std::wstring currentPanelDir() {
    const auto& psi = PluginContext::Instance().info();
    wchar_t buf[kPanelDirBufSize] = {0};
    psi.Control(PANEL_ACTIVE, FCTL_GETPANELDIR, static_cast<int>(kPanelDirBufSize),
                reinterpret_cast<LONG_PTR>(buf));
    return std::wstring(buf);
}

inline const PluginStartupInfo& Psi() {
    return PluginContext::Instance().info();
}

} // namespace

Plugin::Plugin() : aliasMgr_(std::make_unique<AliasManager>(aliasesPath())) {
    if (auto r = aliasMgr_->load(); !r) {
        ErrorLogger::writeToFile(r.error());
    }
}

void Plugin::showInfo(MsgID title, MsgID body, const std::vector<std::wstring>& args) const {
    const std::wstring t = GetMsg(title);
    const std::wstring b = FormatMsg(body, args);
    const wchar_t* items[] = {t.c_str(), b.c_str()};
    Psi().Message(Psi().ModuleNumber, FMSG_MB_OK, nullptr, items, 2, 0);
}

bool Plugin::confirm(MsgID title, MsgID body, const std::vector<std::wstring>& args) const {
    const std::wstring t = GetMsg(title);
    const std::wstring b = FormatMsg(body, args);
    const wchar_t* items[] = {t.c_str(), b.c_str()};
    const int r = Psi().Message(Psi().ModuleNumber, FMSG_MB_YESNO, nullptr, items, 2, 0);
    return r == 0;
}

void Plugin::navigateTo(const std::wstring& path) const {
    Psi().Control(PANEL_ACTIVE, FCTL_SETPANELDIR, 0, reinterpret_cast<LONG_PTR>(path.c_str()));

    PanelRedrawInfo pri{};
    Psi().Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, reinterpret_cast<LONG_PTR>(&pri));
}

HANDLE Plugin::openAliasPanel(std::wstring title, std::vector<std::wstring> names,
                              std::vector<std::wstring> paths) const {
    auto* data = new PanelData{};
    data->title = std::move(title);
    data->names = std::move(names);
    data->paths = std::move(paths);

    // Считаем максимальную ширину имени для выравнивания.
    std::size_t maxNameLen = 0;
    for (const auto& n : data->names) {
        maxNameLen = std::max(maxNameLen, n.size());
    }

    // Формируем «имя  →  путь», имя дополнено пробелами до maxNameLen.
    data->displayNames.reserve(data->names.size());
    for (std::size_t i = 0; i < data->names.size(); ++i) {
        std::wstring display = data->names[i];
        if (display.size() < maxNameLen) {
            display.append(maxNameLen - display.size(), L' ');
        }
        display += L"  →  ";
        display += (i < data->paths.size()) ? data->paths[i] : L"";
        data->displayNames.push_back(std::move(display));
    }

    return reinterpret_cast<HANDLE>(data);
}

void Plugin::handleCreate(const std::wstring& name) {
    const std::wstring dir = currentPanelDir();
    if (dir.empty()) {
        ErrorLogger::log(Error::make(MsgID::CannotDetermineCurrentDir));
        return;
    }
    if (auto r = aliasMgr_->upsert(name, dir); !r) {
        ErrorLogger::log(r.error());
        return;
    }
    showInfo(MsgID::PluginTitle, MsgID::AliasCreated, {name, dir});
}

void Plugin::handleNavigate(const std::wstring& name) {
    auto r = aliasMgr_->find(name);
    if (!r) {
        ErrorLogger::log(r.error());
        return;
    }
    navigateTo((*r)->path);
}

void Plugin::handlePrefixSearch(const std::wstring& prefix) {
    auto matches = aliasMgr_->findByPrefix(prefix);
    if (matches.empty()) {
        ErrorLogger::log(Error::make(MsgID::NoMatchingAliases, ErrorSeverity::Error, {prefix}));
        return;
    }
    if (matches.size() == 1) {
        navigateTo(matches.front()->path);
        return;
    }
    std::vector<FarMenuItem> items;
    items.reserve(matches.size());
    for (const auto* a : matches) {
        FarMenuItem it{};
        it.Text = a->name.c_str();
        items.push_back(it);
    }
    const int choice =
        Psi().Menu(Psi().ModuleNumber, -1, -1, 0, FMENU_WRAPMODE, GetMsg(MsgID::MatchMenuTitle),
                   GetMsg(MsgID::MatchMenuFooter), L"nf", nullptr, nullptr, items.data(),
                   static_cast<int>(items.size()));
    if (choice >= 0 && choice < static_cast<int>(matches.size())) {
        navigateTo(matches[choice]->path);
    }
}

HANDLE Plugin::HandleCommand(const std::wstring& rest) {
    // cd:            -> панель со всеми алиасами
    // cd::name       -> создать алиас name для текущего каталога
    // cd:name        -> перейти к алиасу name (или префиксный поиск)
    if (rest.empty()) {
        std::vector<std::wstring> names;
        std::vector<std::wstring> paths;
        for (const auto& a : aliasMgr_->all()) {
            names.push_back(a.name);
            paths.push_back(a.path);
        }
        return openAliasPanel(GetMsg(MsgID::AliasPanelTitle), std::move(names), std::move(paths));
    }

    if (rest.front() == L':') {
        handleCreate(rest.substr(1));
        return INVALID_HANDLE_VALUE;
    }

    if (auto r = aliasMgr_->find(rest); r) {
        navigateTo((*r)->path);
        return INVALID_HANDLE_VALUE;
    }

    handlePrefixSearch(rest);
    return INVALID_HANDLE_VALUE;
}

} // namespace nf