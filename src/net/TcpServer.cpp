#include "net/TcpServer.h"

#include "net/SocketUtil.h"
#include "protocol/Command.h"
#include "protocol/Protocol.h"

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

    std:: string data(buffer,static_cast<std::size_t>(n));

    Packet request;
    if (!Protocol::parsePacket(data, request)) {
        std::cerr << "[tcp_server] decode request failed" << std::endl;
        
        Packet response = Protcool::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "bad packet"
        );

        std::string encoded = Protcool::encode(response);
        ::send(client_fd, encoded.data(), encoded.size(), 0);
        return;
    }

    std::cout << "[tcp_server] packet received"
              << ", cmd=" << static_cast<int>(request.header.cmd)
              << ", status=" << static_cast<int>(request.header.status)
              << ", body_length=" << request.header.body_length
              << ", body=" << request.body
              << std::endl;

    /*
     * 当前只处理 PING。
     *
     * 后续会在这里分发：
     * STORAGE_JOIN -> TrackerService::handleStorageJoin
     * STORAGE_HEARTBEAT -> TrackerService::handleHeartbeat
     * QUERY_UPLOAD_STORAGE -> TrackerService::handleQueryUpload
     */
    Packet response;
    if(request.header.cmd == Command::PING) {
        response = Protcool::makePacket(
            Command::RESPONSE,
            Status::OK,
            "PONG from tracker"
        );
    } else {
        response = Protcool::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "unknown command"
        );
    }

    std::string encoded = Protcool::encode(response);
    
    ssize_t sent = ::send(client_fd, encoded.data(), encoded.size(), 0);
    if (sent < 0) {
        std::cerr << "[tcp_server] send failed: "
                  << std::strerror(errno) << std::endl;
    }
}