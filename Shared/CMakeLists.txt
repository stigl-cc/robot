aux_source_directory(src SHARED_SOURCES)

add_library(shared ${SHARED_SOURCES})
target_include_directories(shared PUBLIC inc)
target_compile_options(shared PRIVATE -Wall -Wextra -Wpedantic -Wno-missing-designated-field-initializers -Wno-missing-field-initializers -Wold-style-cast)
