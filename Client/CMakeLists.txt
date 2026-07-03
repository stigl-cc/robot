cmake_minimum_required(VERSION 3.31)
project(RobotClient VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SOURCE_FILE_EXTENSIONS cpp;cc)

# Compile shared separately since we are cross compiling
add_subdirectory(${CMAKE_SOURCE_DIR}/../Shared ${CMAKE_BINARY_DIR}/shared)

aux_source_directory(src CLIENT_SOURCES)

add_executable(client ${CLIENT_SOURCES})
target_include_directories(client PRIVATE inc)
target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic -Wno-missing-designated-field-initializers -Wno-missing-field-initializers -Wold-style-cast)
target_link_libraries(client PRIVATE -lv4l2 -lv4lconvert shared)
