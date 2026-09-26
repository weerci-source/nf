#include "panel_data.h"

#include <algorithm>
#include <iterator>

namespace nf {

void PanelData::rebuildDisplays() {
    if (entries.empty()) {
        return;
    }

    const std::size_t maxNameLen = std::ranges::max_element(entries, std::less{}, [](const PanelEntry& e) { return e.name.size(); })->name.size();

    for (auto& e : entries) {
        e.display = e.name;
        e.display.append(maxNameLen - e.name.size(), L' ');
        e.display += L"  →  ";
        e.display += e.path;
    }
}

bool PanelData::eraseAt(std::size_t idx) {
    if (idx >= entries.size()) {
        return false;
    }
    entries.erase(std::next(entries.begin(), static_cast<std::ptrdiff_t>(idx)));
    rebuildDisplays();
    return true;
}

std::optional<std::wstring> PanelData::pathForDisplay(std::wstring_view s) const {
    auto it = std::ranges::find_if(entries, [s](const PanelEntry& e) { return e.display == s || e.name == s; });
    if (it == entries.end()) {
        return std::nullopt;
    }
    return it->path;
}

std::optional<std::wstring> PanelData::nameAt(std::size_t idx) const {
    if (idx >= entries.size()) {
        return std::nullopt;
    }
    return entries[idx].name;
}

std::optional<std::wstring> PanelData::displayAt(std::size_t idx) const {
    if (idx >= entries.size()) {
        return std::nullopt;
    }
    return entries[idx].display;
}

} // namespace nf