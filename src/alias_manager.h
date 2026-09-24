#pragma once

#include "alias.h"
#include "error.h"
#include <filesystem>

namespace nf {

class AliasManager {
  public:
    explicit AliasManager(std::filesystem::path storageFile);

    void_err load();
    [[nodiscard]] void_err save() const;

    const std::vector<Alias>& all() const { return aliases_; }

    // Поиск: возвращает t_err<const Alias*>, чтобы вызывающий различал
    // «не найдено» и «пустой указатель по другой причине».
    [[nodiscard]] t_err<const Alias*> find(const std::wstring& name) const;
    [[nodiscard]] std::vector<const Alias*> findByPrefix(const std::wstring& prefix) const;

    void_err add(const std::wstring& name, const std::wstring& path);
    void_err upsert(const std::wstring& name, const std::wstring& path);
    void_err remove(const std::wstring& name);

    [[nodiscard]] const std::filesystem::path& storageFile() const { return storageFile_; }

  private:
    std::filesystem::path storageFile_;
    std::vector<Alias> aliases_;
};

} // namespace nf