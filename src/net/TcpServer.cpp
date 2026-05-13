#include "net/TcpServer.h"

#include "net/SocketUtil.h"
#include "protocol/Command.h"
#include "protocol/Protocol.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

const uint64_t kMaxPacketBodySize = 64ULL * 1024ULL * 1024ULL;

bool sendAll(int fd, const char* data, std::size_t size)
{
    std::size_t sent_total = 0;
    while (sent_total < size) {
        ssize_t sent = ::send(fd, data + sent_total, size - sent_total, 0);
        if (sent < 0 && errno == EINTR) {
            continue;
        }
        if (sent <= 0) {
            return false;
        }
        sent_total += static_cast<std::size_t>(sent);
    }
    return true;
}

bool recvAll(int fd, char* data, std::size_t size)
{
    std::size_t received_total = 0;
    while (received_total < size) {
        ssize_t n = ::recv(fd, data + received_total, size - received_total, 0);
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n <= 0) {
            return false;
        }
        received_total += static_cast<std::size_t>(n);
    }
    return true;
}

bool recvPacket(int fd, Packet* packet)
{
    if (packet == nullptr) {
        return false;
    }

    std::string data(Protocol::HEADER_SIZE, '\0');
    if (!recvAll(fd, &data[0], data.size())) {
        return false;
    }

    PacketHeader header;
    if (!Protocol::decodeHeader(data, &header)) {
        return false;
    }

    if (header.body_length > kMaxPacketBodySize) {
        std::cerr << "[tcp_server] packet body too large: "
                  << header.body_length << std::endl;
        return false;
    }

    data.resize(Protocol::HEADER_SIZE + static_cast<std::size_t>(header.body_length));
    if (header.body_length > 0) {
        if (!recvAll(fd,
                     &data[Protocol::HEADER_SIZE],
                     static_cast<std::size_t>(header.body_length))) {
            return false;
        }
    }

    return Protocol::decode(data, packet);
}

} // namespace

TcpServer::TcpServer(const std::string& ip, int port)
    : ip_(ip), port_(port) {}

void TcpServer::setPacketHandler(PacketHandler handler) {
    packet_handler_ = handler;
}
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
        sockaddr_in client_addr;
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

        /*
         * 把客户端 IP 从二进制格式转成字符串格式。
         *
         * 例如：
         * 127.0.0.1
         */
        char ip_buffer[INET_ADDRSTRLEN];
        std::memset(ip_buffer, 0, sizeof(ip_buffer));

        const char* ip_result = ::inet_ntop(AF_INET, &client_addr.sin_addr, ip_buffer, sizeof(ip_buffer));

        std::string peer_ip = ip_result ? ip_buffer : "unknown";

        std::cout << "[tcp_server] client connected"
                  << ", fd=" << client_fd
                  << ", ip=" << peer_ip
                  << std::endl;

        handleClient(client_fd, peer_ip);

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

void TcpServer::handleClient(int client_fd, const std::string& peer_ip)
{
    Packet request;
    if (!recvPacket(client_fd, &request)) {
        std::cerr << "[tcp_server] decode request failed" << std::endl;
        
        Packet response = Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "bad packet"
        );

        std::string encoded = Protocol::encode(response);
        sendAll(client_fd, encoded.data(), encoded.size());
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

     /*
     * 如果设置了业务回调，就交给业务层处理。
     *
     * 对 tracker 来说，这里会调用 TrackerService::handlePacket。
     */
    if(packet_handler_) {
        response = packet_handler_(request, peer_ip);
    } else
    {
        /*
         * 如果没有设置业务回调，提供一个默认 PING 处理。
         * 这样 TcpServer 本身仍然可单独测试。
         */
        if(request.header.cmd == Command::PING) {
            response = Protocol::makePacket(
                Command::RESPONSE,
                Status::OK,
                "PONG"
            );
        } else {
            response = Protocol::makePacket(
                Command::RESPONSE,
                Status::ERROR,
                "no packet handler"
            );
        }
    }

    std::string encoded = Protocol::encode(response);
    
    if (!sendAll(client_fd, encoded.data(), encoded.size())) {
        std::cerr << "[tcp_server] send failed: "
                  << std::strerror(errno) << std::endl;
    }
}
