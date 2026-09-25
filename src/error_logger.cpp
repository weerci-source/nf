
#include <chrono>
#include <ctime>
#include <mutex>
#include <sstream>
#include "common.h"

#include "error_logger.h"
#include "plugin_context.h"

namespace nf {

std::mutex ErrorLogger::mtx_;
std::ofstream ErrorLogger::file_;
std::filesystem::path ErrorLogger::logPath_;
LogOutput ErrorLogger::output_ = LogOutput::Both;
ErrorSeverity ErrorLogger::minSeverity_ = ErrorSeverity::Error;

void ErrorLogger::init(const std::filesystem::path& logFile, LogOutput output,
                       ErrorSeverity minSeverity) {
    std::lock_guard lock(mtx_);
    if (file_.is_open())
        file_.close();
    logPath_ = logFile;
    output_ = output;
    minSeverity_ = minSeverity;
}

void ErrorLogger::setOutput(LogOutput output) {
    std::lock_guard lock(mtx_);
    output_ = output;
}

void ErrorLogger::setMinSeverity(ErrorSeverity severity) {
    std::lock_guard lock(mtx_);
    minSeverity_ = severity;
}

LogOutput ErrorLogger::output() {
    std::lock_guard lock(mtx_);
    return output_;
}

ErrorSeverity ErrorLogger::minSeverity() {
    std::lock_guard lock(mtx_);
    return minSeverity_;
}

void ErrorLogger::log(const Error& error) {
    // Снимок настроек под мьютексом, вывод — уже без него.
    LogOutput mode;
    ErrorSeverity minSev;
    {
        std::lock_guard lock(mtx_);
        mode = output_;
        minSev = minSeverity_;
    }

    if (error.severity < minSev)
        return;

    if (has(mode, LogOutput::File))
        writeToFile(error);
    if (has(mode, LogOutput::Message))
        showDialog(error);
}

void ErrorLogger::writeToFile(const Error& error) {
    std::lock_guard lock(mtx_);
    ensureFileOpenLocked();
    if (!file_.is_open())
        return; // тихо игнорируем — некуда писать
    file_ << formatForFile(error);
    file_.flush();
}

void ErrorLogger::showDialog(const Error& error) {
    // Если плагин ещё не инициализирован (например, ошибка в SetStartupInfoW
    // до PluginContext::Init) — не падаем, просто ничего не делаем.
    if (!PluginContext::Instance().initialized())
        return;

    const std::wstring body = error.localizedMessage();
    const wchar_t* items[] = {GetMsg(MsgID::ErrorTitle), body.c_str()};

    const auto& psi = PluginContext::Instance().info();
    psi.Message(psi.ModuleNumber, FMSG_MB_OK, nullptr, items, 2, 0);
}

void ErrorLogger::ensureFileOpenLocked() {
    if (file_.is_open() || logPath_.empty())
        return;

    std::error_code ec;
    if (!logPath_.parent_path().empty()) {
        std::filesystem::create_directories(logPath_.parent_path(), ec);
    }
    file_.open(logPath_, std::ios::out | std::ios::app);
}

std::string ErrorLogger::formatForFile(const Error& error) {
    std::ostringstream oss;
    oss << "[" << timestamp() << "] "
        << "[" << severityTag(error.severity) << "] " << toUtf8(error.localizedMessage());

    if (error.severity == ErrorSeverity::Critical) {
        oss << " | " << toUtf8(error.stackTraceToString());
    }
    oss << '\n';
    return oss.str();
}

const char* ErrorLogger::severityTag(ErrorSeverity sev) {
    switch (sev) {
    case ErrorSeverity::Info:
        return "INFO";
    case ErrorSeverity::Warning:
        return "WARN";
    case ErrorSeverity::Error:
        return "ERROR";
    case ErrorSeverity::Critical:
        return "CRITICAL";
    }
    return "?";
}

std::string ErrorLogger::timestamp() {
    using namespace std::chrono;
    const auto t = system_clock::to_time_t(system_clock::now());
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

} // namespace nf