#pragma once
#include <farplug-wide.h>

#include <memory>
#include <string>
#include <vector>

#include "alias_manager.h"
#include "error.h"
#include "msg_ids.h"

namespace nf {

class Plugin {
  public:
    Plugin(); // без аргументов — контекст уже инициализирован

    HANDLE HandleCommand(const std::wstring& rest);

    AliasManager& aliases() { return *aliasMgr_; }

    void showInfo(MsgID title, MsgID body, const std::vector<std::wstring>& args = {}) const;
    [[nodiscard]] bool confirm(MsgID title, MsgID body,
                               const std::vector<std::wstring>& args = {}) const;
    void navigateTo(const std::wstring& path) const;

    [[nodiscard]] HANDLE openAliasPanel(std::wstring title, std::vector<std::wstring> names,
                                        std::vector<std::wstring> paths) const;

  private:
    std::unique_ptr<AliasManager> aliasMgr_;

    void handleCreate(const std::wstring& name);
    void handleNavigate(const std::wstring& name);
    void handlePrefixSearch(const std::wstring& prefix);
};

} // namespace nf