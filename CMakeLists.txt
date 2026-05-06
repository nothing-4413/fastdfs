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

# 公共模块源码
set(COMMON_SRC
    src/common/Config.cpp
)

# 网络模块源码
set(NET_SRC
    src/net/SocketUtil.cpp
    src/net/TcpServer.cpp
    src/net/TcpClient.cpp
)

add_executable(tracker_server
    src/main/tracker_main.cpp
    ${COMMON_SRC}
    ${NET_SRC}
)

add_executable(storage_server
    src/main/storage_main.cpp
    ${COMMON_SRC}
    ${NET_SRC}
)

add_executable(fdfs_cli
    src/client/fdfs_cli.cpp
    ${COMMON_SRC}
    ${NET_SRC}
)
