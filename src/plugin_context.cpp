#include "plugin_context.h"
#include "common.h"

namespace nf {

void PluginContext::Init(const PluginStartupInfo& info) {
    PluginContext& ctx = Instance();

    ctx.info_ = info;

    if (info.FSF) {
        ctx.fsf_ = *info.FSF;
        ctx.info_.FSF = &ctx.fsf_;
    }

    ctx.initialized_ = true;
}

PluginContext& PluginContext::Instance() {
    static PluginContext s_ctx;
    return s_ctx;
}

// ---- Методы класса ----

const wchar_t* PluginContext::GetMsg(MsgID id) const { 
    if (!initialized_) {
        return L"<nf: PluginContext not initialized>";
    }
    return info_.GetMsg(info_.ModuleNumber, static_cast<int>(id));
}

std::wstring PluginContext::FormatMsg(MsgID id,
                                      const std::vector<std::wstring>& args) const {
    return substitutePlaceholders(GetMsg(id), args);
}

// ---- Free-функции (обёртки) ----
const wchar_t* GetMsg(MsgID id) {
    return PluginContext::Instance().GetMsg(id);
}

std::wstring FormatMsg(MsgID id, const std::vector<std::wstring>& args) {
    return PluginContext::Instance().FormatMsg(id, args);
}

} // namespace nf