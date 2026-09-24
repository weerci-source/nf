#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include "msg_ids.h"

namespace nf {

enum class ErrorSeverity : std::uint8_t { Info, Warning, Error, Critical };

struct Error {
    MsgID msg_id;
    ErrorSeverity severity = ErrorSeverity::Error;
    std::vector<std::wstring> args;
    const char* file = "";
    int line = 0;
    const char* function = "";

    // Базовая фабрика: без trace-информации. Для внутренних вызовов,
    // где источник ошибки и так очевиден.
    static Error make(MsgID msg_id, ErrorSeverity sev = ErrorSeverity::Error,
                      std::vector<std::wstring> args = {}) {
        return Error{msg_id, sev, std::move(args)};
    }

    // Фабрика с trace-информацией. Не зовите руками —
    // используйте макросы NF_ERR / NF_ERR0 ниже.
    static Error make_traced(MsgID msg_id, ErrorSeverity sev, std::vector<std::wstring> args,
                             const char* file, int line, const char* function) {
        return Error{msg_id, sev, std::move(args), file, line, function};
    }

    [[nodiscard]] std::wstring localizedMessage() const;
    [[nodiscard]] std::wstring stackTraceToString() const;
};

template <typename T> using t_err = std::expected<T, Error>;

using void_err = std::expected<void, Error>;

} // namespace nf

// ---- Макросы автоматического захвата места вызова ----
// NF_ERR0  — без аргументов подстановки
// NF_ERR   — с аргументами {0}, {1}, ...
// NF_ERR_SEV — с явной severity

#define NF_ERR0(msg_id)                                                                            \
    ::nf::Error::make_traced(msg_id, ::nf::ErrorSeverity::Error, {}, __FILE__, __LINE__, __func__)

#define NF_ERR(msg_id, ...)                                                                        \
    ::nf::Error::make_traced(msg_id, ::nf::ErrorSeverity::Error,                                   \
                             ::std::vector<::std::wstring>{__VA_ARGS__}, __FILE__, __LINE__,       \
                             __func__)

#define NF_ERR_SEV(msg_id, sev, ...)                                                               \
    ::nf::Error::make_traced(msg_id, sev, ::std::vector<::std::wstring>{__VA_ARGS__}, __FILE__,    \
                             __LINE__, __func__)