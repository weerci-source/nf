#include "alias_manager.h"
#include "common.h"
#include <algorithm>
#include <fstream>

namespace nf {

namespace {

void_err validateName(const std::wstring& name) {
    if (name.empty()) {
        return std::unexpected(Error::make(MsgID::InvalidAliasNameEmpty));
    }
    if (name.find(L':') != std::wstring::npos) {
        return std::unexpected(Error::make(MsgID::InvalidAliasNameColon));
    }
    return {};
}

std::wstring normalizeAliasName(const std::wstring& s) {
    std::wstring out = s;
    for (wchar_t& c : out) {
        // ASCII A-Z -> a-z
        if (c >= L'A' && c <= L'Z') {
            c += (L'a' - L'A');
            continue;
        }
        // Кириллица А-Я (U+0410..U+042F) -> а-я (U+0430..U+044F)
        if (c >= 0x0410 && c <= 0x042F) {
            c += 0x20;
            continue;
        }
        // Ё (U+0401) -> ё (U+0451) — вне основного блока
        if (c == 0x0401) {
            c = 0x0451;
            continue;
        }
    }
    return out;
}

} // namespace

AliasManager::AliasManager(std::filesystem::path storageFile)
    : storageFile_(std::move(storageFile)) {}

void_err AliasManager::load() {
    aliases_.clear();
    std::error_code ec;
    if (!std::filesystem::exists(storageFile_, ec)) {
        return {}; // отсутствие файла — не ошибка
    }

    std::ifstream in(storageFile_, std::ios::binary);
    if (!in) {
        return std::unexpected(
            Error::make(MsgID::StorageReadFailed, ErrorSeverity::Error, {storageFile_.wstring()}));
    }

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto sep = line.find('\t');
        if (sep == std::string::npos) {
            continue; // битая строка — пропускаем
        }
        Alias a;
        a.name = normalizeAliasName(fromUtf8(line.substr(0, sep)));
        a.path = fromUtf8(line.substr(sep + 1));
        aliases_.push_back(std::move(a));
    }
    return {};
}

void_err AliasManager::save() const {
    std::error_code ec;
    if (!storageFile_.parent_path().empty()) {
        std::filesystem::create_directories(storageFile_.parent_path(), ec);
    }

    std::ofstream out(storageFile_, std::ios::binary | std::ios::trunc);
    if (!out) {
        return std::unexpected(
            Error::make(MsgID::StorageWriteFailed, ErrorSeverity::Error, {storageFile_.wstring()}));
    }

    out << "# nf aliases: name<TAB>path\n";
    for (const auto& a : aliases_) {
        out << toUtf8(a.name) << '\t' << toUtf8(a.path) << '\n';
    }
    return {};
}

t_err<const Alias*> AliasManager::find(const std::wstring& name) const {
    const std::wstring needle = normalizeAliasName(name);
    for (const auto& a : aliases_) {
        if (a.name == needle) {
            return &a;
        }
    }
    return std::unexpected(Error::make(MsgID::AliasNotFound, ErrorSeverity::Error, {name}));
}

std::vector<const Alias*> AliasManager::findByPrefix(const std::wstring& prefix) const {
    const std::wstring needle = normalizeAliasName(prefix);
    std::vector<const Alias*> result;
    for (const auto& a : aliases_) {
        if (a.name.size() >= needle.size() && a.name.compare(0, needle.size(), needle) == 0) {
            result.push_back(&a);
        }
    }
    return result;
}

void_err AliasManager::add(const std::wstring& name, const std::wstring& path) {
    if (auto v = validateName(name); !v) {
        return v;
    }
    const std::wstring key = normalizeAliasName(name);
    if (auto f = find(key); f) {
        return std::unexpected(
            Error::make(MsgID::AliasAlreadyExists, ErrorSeverity::Error, {name}));
    }
    aliases_.push_back({key, path});
    return save();
}

void_err AliasManager::upsert(const std::wstring& name, const std::wstring& path) {
    const std::wstring key = normalizeAliasName(name);
    if (auto v = validateName(key); !v) {
        return v;
    }
    for (auto& a : aliases_) {
        if (a.name == key) {
            a.path = path;
            return save();
        }
    }
    aliases_.push_back({key, path});
    return save();
}

void_err AliasManager::remove(const std::wstring& name) {
    const std::wstring key = normalizeAliasName(name);
    auto it = std::remove_if(aliases_.begin(), aliases_.end(),
                             [&](const Alias& a) { return a.name == key; });
    if (it == aliases_.end()) {
        return std::unexpected(Error::make(MsgID::AliasNotFound, ErrorSeverity::Error, {name}));
    }
    aliases_.erase(it, aliases_.end());
    return save();
}

} // namespace nf