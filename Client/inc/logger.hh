#pragma once

#include <cstring>
#include <iostream>
#include <string_view>

enum LogLevels {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERR,
};

const std::string_view LOGLEVEL_STRS[] = {
    "Info",
    "Warning",
    "Error",
};

#define log_tag_no(level, subject)                                      \
    std::cerr << LOG_TAG << ": " << LOGLEVEL_STRS[level] << " of " << subject << " with " << strerror(errno) << std::endl

#define log_tag(level, subject)                                         \
    std::cerr << LOG_TAG << ": " << LOGLEVEL_STRS[level] << " of " << subject << std::endl

#define log_no(level, subject)                                          \
    std::cerr << LOGLEVEL_STRS[level] << " of " << subject << " with " << strerror(errno) << std::endl

#define log(level, subject)                                             \
    std::cerr << LOGLEVEL_STRS[level] << " of " << subject << std::endl
