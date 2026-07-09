cmake_minimum_required(VERSION 3.31)
project(RobotServer VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SOURCE_FILE_EXTENSIONS cpp;cc)

find_package(llama REQUIRED)

# Compile shared separately since we are cross compiling
add_subdirectory(${CMAKE_SOURCE_DIR}/../Shared ${CMAKE_BINARY_DIR}/shared)

if (NOT DEFINED LLAMA_INSTALL_VERSION)
    set(LLAMA_INSTALL_VERSION "1.0.0")
endif()
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/llama.cpp/tools/mtmd ${CMAKE_BINARY_DIR}/llama.cpp-mtmd)
set_target_properties(mtmd PROPERTIES COMPILE_FLAGS "-w")

aux_source_directory(src SERVER_SOURCES)

add_executable(server ${SERVER_SOURCES})
target_include_directories(server PRIVATE inc)
target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic -Wno-missing-designated-field-initializers -Wno-missing-field-initializers -Wold-style-cast)
target_link_libraries(server PRIVATE shared llama mtmd)
