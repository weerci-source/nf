#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <random>

#include "alias_manager.h"

namespace {

// Уникальный временный файл для каждого теста.
// Catch2 запускает тесты последовательно, так что коллизий нет.
class TempAliasFile {
  public:
    TempAliasFile() {
        static std::mt19937_64 rng{std::random_device{}()};
        dir_ = std::filesystem::temp_directory_path() / "nf_tests";
        std::filesystem::create_directories(dir_);
        path_ = dir_ / ("aliases_" + std::to_string(rng()) + ".txt");
    }
    ~TempAliasFile() {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }
    const std::filesystem::path& path() const { return path_; }

  private:
    std::filesystem::path dir_;
    std::filesystem::path path_;
};

} // namespace

// ==================== load ====================

TEST_CASE("AliasManager::load: отсутствие файла — не ошибка", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    CHECK(mgr.load());
    CHECK(mgr.all().empty());
}

TEST_CASE("AliasManager::load: битые строки пропускаются", "[alias_manager]") {
    TempAliasFile tmp;
    {
        std::ofstream out(tmp.path(), std::ios::binary);
        out << "# comment\n";
        out << "good\t/tmp\n";
        out << "no-tab-here\n"; // пропускается
        out << "\n";            // пропускается
        out << "another\t/var\n";
    }
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.all().size() == 2);
    CHECK(mgr.all()[0].name == L"good");
    CHECK(mgr.all()[1].name == L"another");
}

TEST_CASE("AliasManager::load: CRLF нормализуется", "[alias_manager]") {
    TempAliasFile tmp;
    {
        std::ofstream out(tmp.path(), std::ios::binary);
        out << "foo\t/tmp\r\n";
        out << "bar\t/var\r\n";
    }
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.all().size() == 2);
    CHECK(mgr.all()[0].path == L"/tmp");
    CHECK(mgr.all()[1].path == L"/var");
}

// ==================== save/load round-trip ====================

TEST_CASE("AliasManager: save/load round-trip", "[alias_manager]") {
    TempAliasFile tmp;
    {
        nf::AliasManager mgr(tmp.path());
        REQUIRE(mgr.load());
        REQUIRE(mgr.add(L"дом", L"/home/user"));
        REQUIRE(mgr.add(L"работа", L"/var/work"));
    }
    {
        nf::AliasManager mgr(tmp.path());
        REQUIRE(mgr.load());
        REQUIRE(mgr.all().size() == 2);
        CHECK(mgr.all()[0].name == L"дом");
        CHECK(mgr.all()[0].path == L"/home/user");
        CHECK(mgr.all()[1].name == L"работа");
        CHECK(mgr.all()[1].path == L"/var/work");
    }
}

// ==================== add ====================

TEST_CASE("AliasManager::add: пустое имя отклоняется", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    auto r = mgr.add(L"", L"/tmp");
    REQUIRE_FALSE(r);
    CHECK(r.error().msg_id == nf::MsgID::InvalidAliasNameEmpty);
}

TEST_CASE("AliasManager::add: имя с ':' отклоняется", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    auto r = mgr.add(L"foo:bar", L"/tmp");
    REQUIRE_FALSE(r);
    CHECK(r.error().msg_id == nf::MsgID::InvalidAliasNameColon);
}

TEST_CASE("AliasManager::add: дубликат отклоняется", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"foo", L"/tmp"));
    auto r = mgr.add(L"foo", L"/var");
    REQUIRE_FALSE(r);
    CHECK(r.error().msg_id == nf::MsgID::AliasAlreadyExists);
    // Старый путь не должен перезаписаться
    auto found = mgr.find(L"foo");
    REQUIRE(found);
    CHECK((*found)->path == L"/tmp");
}

TEST_CASE("AliasManager::add: дубликат по регистру", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"foo", L"/tmp"));
    auto r = mgr.add(L"FOO", L"/var");
    REQUIRE_FALSE(r);
    CHECK(r.error().msg_id == nf::MsgID::AliasAlreadyExists);
}

// ==================== find ====================

TEST_CASE("AliasManager::find: регистронезависимый (ASCII)", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"Foo", L"/tmp"));

    CHECK(mgr.find(L"foo"));
    CHECK(mgr.find(L"FOO"));
    CHECK(mgr.find(L"Foo"));
    CHECK(mgr.find(L"fOo"));
}

TEST_CASE("AliasManager::find: русский регистр", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"Дом", L"/tmp"));

    CHECK(mgr.find(L"дом"));
    CHECK(mgr.find(L"ДОМ"));
    CHECK(mgr.find(L"доМ"));
}

TEST_CASE("AliasManager::find: Ё/ё нормализуется", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"Ёлка", L"/tmp"));

    CHECK(mgr.find(L"ёлка"));
    CHECK(mgr.find(L"ЁЛКА"));
}

TEST_CASE("AliasManager::find: не найдено", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    auto r = mgr.find(L"nope");
    REQUIRE_FALSE(r);
    CHECK(r.error().msg_id == nf::MsgID::AliasNotFound);
}

// ==================== findByPrefix ====================

TEST_CASE("AliasManager::findByPrefix: находит несколько", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"work", L"/w"));
    REQUIRE(mgr.add(L"workspace", L"/ws"));
    REQUIRE(mgr.add(L"home", L"/h"));

    CHECK(mgr.findByPrefix(L"work").size() == 2);
    CHECK(mgr.findByPrefix(L"work").size() == 2);
    CHECK(mgr.findByPrefix(L"h").size() == 1);
    CHECK(mgr.findByPrefix(L"xyz").empty());
}

TEST_CASE("AliasManager::findByPrefix: регистронезависимый", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"Work", L"/w"));

    CHECK(mgr.findByPrefix(L"work").size() == 1);
    CHECK(mgr.findByPrefix(L"WORK").size() == 1);
}

// ==================== upsert ====================

TEST_CASE("AliasManager::upsert: обновляет существующий", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"foo", L"/old"));
    REQUIRE(mgr.upsert(L"foo", L"/new"));

    REQUIRE(mgr.all().size() == 1);
    auto r = mgr.find(L"foo");
    REQUIRE(r);
    CHECK((*r)->path == L"/new");
}

TEST_CASE("AliasManager::upsert: добавляет новый", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.upsert(L"new", L"/n"));
    CHECK(mgr.all().size() == 1);
}

// ==================== remove (регрессия) ====================

TEST_CASE("AliasManager::remove: удаляет существующий", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"foo", L"/tmp"));
    REQUIRE(mgr.remove(L"foo"));
    CHECK(mgr.all().empty());
}

TEST_CASE("AliasManager::remove: несуществующий → AliasNotFound", "[alias_manager][regression]") {
    // Регрессия: раньше проверялось it == aliases_.end(), что работало
    // случайно. Теперь сравниваем размеры.
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    auto r = mgr.remove(L"nope");
    REQUIRE_FALSE(r);
    CHECK(r.error().msg_id == nf::MsgID::AliasNotFound);
}

TEST_CASE("AliasManager::remove: несуществующий в непустом списке", "[alias_manager][regression]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"a", L"/a"));
    REQUIRE(mgr.add(L"b", L"/b"));

    auto r = mgr.remove(L"c");
    REQUIRE_FALSE(r);
    CHECK(r.error().msg_id == nf::MsgID::AliasNotFound);
    CHECK(mgr.all().size() == 2); // ничего не удалилось
}

TEST_CASE("AliasManager::remove: регистронезависимо", "[alias_manager]") {
    TempAliasFile tmp;
    nf::AliasManager mgr(tmp.path());
    REQUIRE(mgr.load());
    REQUIRE(mgr.add(L"Foo", L"/tmp"));
    REQUIRE(mgr.remove(L"FOO"));
    CHECK(mgr.all().empty());
}