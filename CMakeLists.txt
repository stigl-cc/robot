cmake_minimum_required(VERSION 3.31)
project(Robot)

include(ExternalProject)

string(REPLACE ";" " " CLIENT_EXE_LINKER_FLAGS "-static-libgcc;-static-libstdc++;-static")

ExternalProject_Add(Server
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/Server/
  BINARY_DIR ${CMAKE_BINARY_DIR}/server
  CMAKE_CACHE_ARGS
  INSTALL_COMMAND ""
  BUILD_ALWAYS 1
)

ExternalProject_Add(Client
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/Client/
  BINARY_DIR ${CMAKE_BINARY_DIR}/client
  LIST_SEPARATOR ,
  CMAKE_CACHE_ARGS
  -DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_SOURCE_DIR}/cmake/aarch64-linux-toolchain.cmake
  -DCMAKE_EXE_LINKER_FLAGS:STRING=${CLIENT_EXE_LINKER_FLAGS}
  INSTALL_COMMAND ""
  BUILD_ALWAYS 1
)

# Client but for the native architecture for testing
ExternalProject_Add(ClientNative
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/Client/
  BINARY_DIR ${CMAKE_BINARY_DIR}/client_native
  CMAKE_CACHE_ARGS
  INSTALL_COMMAND ""
  BUILD_ALWAYS 1
)

return()
