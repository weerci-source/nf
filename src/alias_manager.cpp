#include "alias_manager.h"

#include <algorithm>
#include <codecvt>
#include <fstream>
#include <locale>

namespace nf {

namespace {

std::string toUtf8(const std::wstring& w) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(w);
}

std::wstring fromUtf8(const std::string& s) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(s);
}

void_err validateName(const std::wstring& name) {
    if (name.empty()) {
        return std::unexpected(Error::make(MsgID::InvalidAliasNameEmpty));
    }
    if (name.find(L':') != std::wstring::npos) {
        return std::unexpected(Error::make(MsgID::InvalidAliasNameColon));
    }
    return {};
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
        a.name = fromUtf8(line.substr(0, sep));
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
    for (const auto& a : aliases_) {
        if (a.name == name) {
            return &a;
        }
    }
    return std::unexpected(Error::make(MsgID::AliasNotFound, ErrorSeverity::Error, {name}));
}

std::vector<const Alias*> AliasManager::findByPrefix(const std::wstring& prefix) const {
    std::vector<const Alias*> result;
    for (const auto& a : aliases_) {
        if (a.name.size() >= prefix.size() && a.name.compare(0, prefix.size(), prefix) == 0) {
            result.push_back(&a);
        }
    }
    return result;
}

void_err AliasManager::add(const std::wstring& name, const std::wstring& path) {
    if (auto v = validateName(name); !v) {
        return v;
    }
    if (auto f = find(name); f) {
        return std::unexpected(
            Error::make(MsgID::AliasAlreadyExists, ErrorSeverity::Error, {name}));
    }
    aliases_.push_back({name, path});
    return save();
}

void_err AliasManager::upsert(const std::wstring& name, const std::wstring& path) {
    if (auto v = validateName(name); !v) {
        return v;
    }
    for (auto& a : aliases_) {
        if (a.name == name) {
            a.path = path;
            return save();
        }
    }
    aliases_.push_back({name, path});
    return save();
}

void_err AliasManager::remove(const std::wstring& name) {
    auto it = std::remove_if(aliases_.begin(), aliases_.end(),
                             [&](const Alias& a) { return a.name == name; });
    if (it == aliases_.end()) {
        return std::unexpected(Error::make(MsgID::AliasNotFound, ErrorSeverity::Error, {name}));
    }
    aliases_.erase(it, aliases_.end());
    return save();
}

} // namespace nf