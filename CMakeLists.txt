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

#tracker_server 是 tracker 进程
#后面它会负责 storage 注册、心跳检测、上传/下载调度
add_executable(tracker_server src/main/tracker_server.cpp)

#storage_server 是 storage 进程
#它会负责文件的存储、上传/下载、删除等操作
add_executable(storage_server src/main/storage_server.cpp)

#fds_client 是客户端程序
#它会负责文件的上传/下载/删除等操作
add_executable(fds_client src/client/fds_client.cpp)
