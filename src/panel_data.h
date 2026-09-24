#pragma once
#include <string>
#include <vector>

namespace nf {

struct PanelData {
    std::wstring hostFile = L"nf-aliases";
    std::wstring title;
    std::vector<std::wstring> names;        // чистые имена 
    std::vector<std::wstring> paths;        // пути
    std::vector<std::wstring> displayNames; // «имя  →  путь» (для UI)
};

} // namespace nf