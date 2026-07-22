#include "Core/Log.h"

#include <iostream>
#include <stdexcept>

namespace bhs::core {

const char* Log::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

void Log::log(LogLevel level, const std::string& message) {
    std::ostream& out = (level >= LogLevel::Warning) ? std::cerr : std::cout;
    out << "[" << levelToString(level) << "] " << message << std::endl;
}

void Log::fatal(const std::string& message) {
    log(LogLevel::Fatal, message);
    throw std::runtime_error(message);
}

} // namespace bhs::core
