#pragma once

#include <string>

namespace bhs::core {

enum class LogLevel {
    Trace,
    Info,
    Warning,
    Error,
    Fatal
};

class Log {
public:
    static void log(LogLevel level, const std::string& message);

    static void trace(const std::string& message) { log(LogLevel::Trace, message); }
    static void info(const std::string& message)  { log(LogLevel::Info, message); }
    static void warn(const std::string& message)  { log(LogLevel::Warning, message); }
    static void error(const std::string& message) { log(LogLevel::Error, message); }
    static void fatal(const std::string& message);

private:
    static const char* levelToString(LogLevel level);
};

} // namespace bhs::core
