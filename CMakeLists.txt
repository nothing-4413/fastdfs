cmake_minimum_required(VERSION 3.10)

#TinyFastDFS
#LANGUAGES CXX 表示这是一个c++项目
project(TinyFastDFS LANGUAGES CXX)

#使用c++11标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#添加可执行文件输出到build/bin目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#把src目录下的所有.cpp文件添加到头文件路径中
#代码可以写#include "common/Config.h"
include_directories(${PROJECT_SOURCE_DIR}/src)

find_package(Threads REQUIRED)

set(COMMON_SRC
    src/common/Config.cpp
)

set(PROTOCOL_SRC
    src/protocol/Protocol.cpp
)

set(NET_SRC
    src/net/SocketUtil.cpp
    src/net/TcpServer.cpp
    src/net/TcpClient.cpp
)

set(TRACKER_SRC
    src/tracker/StorageRegistry.cpp
    src/tracker/FileIndex.cpp
    src/tracker/TrackerService.cpp
)

set(STORAGE_SRC
    src/storage/Binlog.cpp
    src/storage/StorageService.cpp
)

add_executable(tracker_server
    src/main/tracker_main.cpp
    ${COMMON_SRC}
    ${PROTOCOL_SRC}
    ${NET_SRC}
    ${TRACKER_SRC}
)

add_executable(storage_server
    src/main/storage_main.cpp
    ${COMMON_SRC}
    ${PROTOCOL_SRC}
    ${NET_SRC}
    ${STORAGE_SRC}
)

add_executable(fdfs_cli
    src/client/fdfs_cli.cpp
    ${COMMON_SRC}
    ${PROTOCOL_SRC}
    ${NET_SRC}
)

target_link_libraries(tracker_server PRIVATE Threads::Threads)
target_link_libraries(storage_server PRIVATE Threads::Threads)
target_link_libraries(fdfs_cli PRIVATE Threads::Threads)
