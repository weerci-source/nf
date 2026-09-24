#pragma once
#include <filesystem>
#include <fstream>
#include <mutex>

#include "error.h"

namespace nf {

enum class LogOutput : std::uint8_t {
    None = 0,
    File = 1,
    Message = 2,
    Both = File | Message,
};

// Разрешает побитовые операции над LogOutput.
inline LogOutput operator|(LogOutput a, LogOutput b) {
    return static_cast<LogOutput>(static_cast<int>(a) | static_cast<int>(b));
}
inline bool has(LogOutput mode, LogOutput flag) {
    return (static_cast<int>(mode) & static_cast<int>(flag)) != 0;
}

class ErrorLogger {
  public:
    // Вызывается один раз из SetStartupInfoW.
    static void init(const std::filesystem::path& logFile, LogOutput output = LogOutput::Both,
                     ErrorSeverity minSeverity = ErrorSeverity::Error);

    // Единая точка обработки ошибки.
    // Фильтрует по minSeverity, затем выводит согласно output.
    static void log(const Error& error);

    // Точечные выводы (без фильтрации, для ad-hoc случаев).
    static void writeToFile(const Error& error);
    static void showDialog(const Error& error);

    // Настройки во время работы.
    static void setOutput(LogOutput output);
    static void setMinSeverity(ErrorSeverity severity);
    static LogOutput output();
    static ErrorSeverity minSeverity();

  private:
    static std::mutex mtx_;
    static std::ofstream file_;
    static std::filesystem::path logPath_;
    static LogOutput output_;
    static ErrorSeverity minSeverity_;

    static void ensureFileOpenLocked();
    static std::string formatForFile(const Error& error);
    static const char* severityTag(ErrorSeverity sev);
    static std::string timestamp();
    static std::string toUtf8(const std::wstring& w);
};

} // namespace nf