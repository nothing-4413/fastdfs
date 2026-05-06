#pragma once

#include <string>

/*
 * SocketUtil
 *
 * 这个文件封装 Linux socket 的基础操作。
 *
 * 为什么要封装？
 *
 * 因为 tracker、storage、client 后面都会用到 socket。
 * 如果每个地方都直接写 socket/bind/listen/connect，
 * 代码会重复，而且后期不好维护。
 *
 * 当前阶段只封装最基础的：
 * 1. 创建监听 socket
 * 2. 创建连接 socket
 * 3. 关闭 socket
 */
namespace SocketUtil
{
/*
 * 创建一个 TCP 监听 socket。
 *
 * 参数：
 * ip：监听的 IP 地址。通常传 "0.0.0.0"，表示监听本机所有网卡。
 * port：监听端口。
 *
 * 返回：
 * 成功返回 listen_fd。
 * 失败返回 -1。
 */
int createListenSocket(const std::string& ip, int port);

/*
 * 创建一个 TCP 客户端连接。
 *
 * 参数：
 * ip：服务端 IP。
 * port：服务端端口。
 *
 * 返回：
 * 成功返回 socket fd。
 * 失败返回 -1。
 */
int connectToServer(const std::string& ip, int port);

/*
 * 关闭 socket。
 *
 * Linux 中 socket 本质上也是文件描述符。
 */
void closeSocket(int fd);
}// namespace SocketUtil