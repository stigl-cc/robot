#pragma once

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>

enum LogLevel {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERR,
};

constexpr const std::string_view LOGLEVEL_STRS[] = {
    "Info",
    "Warning",
    "Error",
};

inline void log(const LogLevel level, std::string_view subject) {
    std::cerr << LOGLEVEL_STRS[level] << " of " << subject << std::endl;
    if(level == LOG_ERR)
        throw std::runtime_error(subject.data());
}

inline void log(const LogLevel level, std::string_view subject, std::string_view tag) {
    std::cerr << tag << ": " << LOGLEVEL_STRS[level] << " of " << subject << std::endl;
    if(level == LOG_ERR)
        throw std::runtime_error(subject.data());
}

inline void log_no(LogLevel level, std::string_view subject) {
    std::cerr << LOGLEVEL_STRS[level] << " of " << subject << " with " << strerror(errno) << std::endl;
    if(level == LOG_ERR)
        throw std::runtime_error(subject.data());
}

inline void log_no(LogLevel level, std::string_view subject, std::string_view tag) {
    std::cerr << tag << ": " << LOGLEVEL_STRS[level] << " of " << subject << " with " << strerror(errno) << std::endl;
    if(level == LOG_ERR)
        throw std::runtime_error(subject.data());
}

#define log_tag(level, subject)                 \
    log(level, subject, LOG_TAG)

#define log_tag_no(level, subject)              \
    log_no(level, subject, LOG_TAG)
