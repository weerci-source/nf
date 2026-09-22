#pragma once

#include <farplug-wide.h>

#include <string>

namespace nf {

// Тонкая C++-обёртка над API far2l.
// Все вызовы идут через psi (PluginStartupInfo), который выдаётся хостом
// в SetStartupInfoW.
class Plugin {
  public:
    explicit Plugin(const PluginStartupInfo& psi);

    // Показать простое меню плагина. Возвращает код выхода.
    int ShowMenu();

    // Простейшее действие для примера.
    void SayHello() const;

  private:
    PluginStartupInfo psi_{};
};

} // namespace nf