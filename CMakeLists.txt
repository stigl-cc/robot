
cmake_minimum_required(VERSION 3.31)
project(Robot)

include(ExternalProject)

ExternalProject_Add(Client
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/Client/
  BINARY_DIR ${CMAKE_BINARY_DIR}/client
  CMAKE_CACHE_ARGS
  -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_SOURCE_DIR}/cmake/aarch64-linux-toolchain.cmake
  INSTALL_COMMAND ""
  BUILD_ALWAYS 1
)
ExternalProject_Add(Server
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/Server/
  BINARY_DIR ${CMAKE_BINARY_DIR}/server
  CMAKE_CACHE_ARGS
  INSTALL_COMMAND ""
  BUILD_ALWAYS 1
)
return()
