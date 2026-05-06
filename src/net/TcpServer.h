#pragma once

#include <string>

/*
 * TcpServer
 *
 * 这是一个最小可用 TCP 服务端。
 *
 * 当前版本特点：
 * 1. 阻塞 accept
 * 2. 一次处理一个客户端连接
 * 3. 收到客户端消息后，返回固定响应
 *
 * 为什么不一开始就写 epoll？
 *
 * 因为这个项目要从 FastDFS 架构逐步复现。
 * 网络层也要先从最基本的 TCP 模型跑通。
 *
 * 后面升级路线：
 * 阻塞 TcpServer
 *   -> 多线程 TcpServer
 *   -> epoll TcpServer
 *   -> Reactor 风格 TcpServer
 */
class TcpServer {
public:
    /*
     * 构造函数。
     *
     * ip：监听地址
     * port：监听端口
     */
    TcpServer(const std::string& ip, int port);

    /*
     * 启动服务端。
     *
     * 成功返回 true。
     * 失败返回 false。
     */
    bool start();

private:
    /*
     * 处理一个客户端连接。
     *
     * 当前只是读取客户端发送的文本，然后返回一段 response。
     */
    void handleClient(int clientFd);

private:
    std::string ip_;
    int port_;
};