#pragma once
#include <farplug-wide.h>

#include <string>
#include <vector>

#include "msg_ids.h"

namespace nf {

class PluginContext {
  public:
    // Инициализация из SetStartupInfoW. Вызывается один раз до всего остального.
    static void Init(const PluginStartupInfo& info);

    // Единая точка доступа.
    static PluginContext& Instance();

    [[nodiscard]] const PluginStartupInfo& info() const { return info_; }
    [[nodiscard]] const FarStandardFunctions& fsf() const { return fsf_; }
    [[nodiscard]] bool initialized() const { return initialized_; }

    // Строка из текущего .lng-файла (язык выбирает far2l).
    [[nodiscard]] const wchar_t* GetMsg(MsgID id) const;

    // Строка с подставленными {0}, {1}, ... .
    [[nodiscard]] std::wstring FormatMsg(MsgID id,
                                         const std::vector<std::wstring>& args = {}) const;

  private:
    PluginContext() = default;

    PluginStartupInfo info_{};
    FarStandardFunctions fsf_{}; // локальная копия; info_.FSF указывает сюда
    bool initialized_ = false;
};

// Удобный синтаксис для мест, где полный путь PluginContext::Instance() избыточен.
const wchar_t* GetMsg(MsgID id);

std::wstring FormatMsg(MsgID id, const std::vector<std::wstring>& args = {});

} // namespace nf