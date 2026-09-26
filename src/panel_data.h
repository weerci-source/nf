#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nf {

// Одна запись панели: имя алиаса, путь и готовая строка для UI.
struct PanelEntry {
    std::wstring name;    // чистое имя ("gamma")
    std::wstring path;    // путь ("/g")
    std::wstring display; // «имя  →  путь» (для GetFindData)
};

struct PanelData {
    std::wstring hostFile = L"nf-aliases";
    std::wstring title;
    std::vector<PanelEntry> entries;

    // Пересобирает display всех записей, выравнивая имена по самой длинной.
    void rebuildDisplays();

    // Удаляет запись по индексу и пересобирает display.
    // Возвращает false, если idx вне диапазона.
    bool eraseAt(std::size_t idx);

    // Все три возвращают std::nullopt, если запись не найдена.
    // Возвращают owning-копию — безопасны после мутаций PanelData.
    //
    // pathForDisplay принимает wstring_view: вызывающий может передать
    // const wchar_t* (от far2l) без создания временного wstring.
    [[nodiscard]] std::optional<std::wstring> pathForDisplay(std::wstring_view s) const;
    [[nodiscard]] std::optional<std::wstring> nameAt(std::size_t idx) const;
    [[nodiscard]] std::optional<std::wstring> displayAt(std::size_t idx) const;
};

} // namespace nf