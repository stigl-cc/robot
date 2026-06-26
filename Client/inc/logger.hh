#pragma once

#define log_tag_no(level, subject)                                      \
    std::cerr << log_tag << ' ' << level << ": Failed to " << subject << " with " << strerror(errno) << std::endl;

#define log_tag(level, subject)                                         \
    std::cerr << log_tag << ' ' << level << ": Failed to " << subject << std::endl;

#define log_no(level, subject)                                          \
    std::cerr << level << ": Failed to " << subject << " with " << strerror(errno) << std::endl;

#define log(level, subject)                                             \
    std::cerr << level << ": Failed to " << subject << std::endl;
