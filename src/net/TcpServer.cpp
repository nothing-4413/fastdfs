#include "net/TcpServer.h"

#include "net/SocketUtil.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

TcpServer::TcpServer(const std::string& ip, int port)
    : ip_(ip), port_(port) {}

/*
 * start
 *
 * 服务端启动流程：
 *
 * 1. socket()
 * 2. setsockopt()
 * 3. bind()
 * 4. listen()
 * 5. accept()
 * 6. recv()
 * 7. send()
 *
 * 前四步被 SocketUtil::createListenSocket 封装。
 */
bool TcpServer::start()
{
    int listen_fd = SocketUtil::createListenSocket(ip_, port_);
    if (listen_fd < 0) {
        return false;
    }

    std::cout << "[tcp_server] listening on "
              << ip_ << ":" << port_ << std::endl;

    /*
     * 当前使用死循环持续 accept。
     *
     * 注意：
     * 这个版本是阻塞模型。
     * 当没有客户端连接时，程序会阻塞在 accept。
     */
    while(true)
    {
        socket_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        std::memset(&client_addr, 0, sizeof(client_addr));

        /*
         * accept()
         *
         * 从监听 socket 中取出一个已经完成三次握手的连接。
         *
         * 返回 client_fd。
         * client_fd 用来和这个客户端通信。
         */
        int client_fd = ::accept(listen_fd,
                                 reinterpret_cast<sockaddr*>(&client_addr),
                                 &client_len);
        
        if(client_fd < 0) {
            std::cerr << "[tcp_server] accept failed: "
                      << std::strerror(errno) << std::endl;
            continue;
        }

        std::cout << "[tcp_server] client connected, fd="
                  << client_fd << std::endl;

        handleClient(client_fd);

        /*
         * 当前版本一次连接只处理一次请求。
         * 处理完就关闭。
         *
         * 后面实现协议和长连接后，这里会变成循环读取 Packet。
         */
        SocketUtil::closeSocket(client_fd);
    }

    /*
     * 理论上当前不会执行到这里。
     */
    SocketUtil::closeSocket(listen_fd);
    return true;
}

void TcpServer::handleClient(int client_fd)
{
    char buffer[1024];

    std::memset(buffer, 0, sizeof(buffer));

    /*
     * recv()
     *
     * 从客户端读取数据。
     *
     * 返回值：
     * > 0：读到的字节数
     * = 0：客户端关闭连接
     * < 0：读取失败
     */
    ssize_t n = ::recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if(n < 0)
    {
        std::cerr << "[tcp_server] recv failed: "
                  << std::strerror(errno) << std::endl;
        return;
    }

    if (n == 0) {
        std::cout << "[tcp_server] client closed connection"<< std::endl;
        return;
    }

    std:: string request(buffer,static_cast<std::size_t>(n));

    std::cout << "[tcp_server] recv: " << request << std::endl;

    /*
     * 当前固定返回一段文本。
     *
     * 下一步实现协议后，这里会根据 cmd 做不同处理：
     * STORAGE_JOIN
     * STORAGE_HEARTBEAT
     * QUERY_UPLOAD_STORAGE
     */
    std::string response = "TinyFastDFS tracker response\n";

    /*
     * send()
     *
     * 向客户端发送响应。
     */
    ssize_t sent = ::send(client_fd, response.data(), response.size(), 0);
    if (sent < 0) {
        std::cerr << "[tcp_server] send failed: "
                  << std::strerror(errno) << std::endl;
    }
}