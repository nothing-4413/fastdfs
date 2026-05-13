#include "net/TcpClient.h"

#include "net/SocketUtil.h"
#include "protocol/Protocol.h"

#include <cerrno>
#include <cstring>
#include <iostream>
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
        std::cerr << "[tcp_client] packet body too large: "
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

TcpClient::TcpClient(const std::string& ip,int port):ip_(ip),port_(port){}

bool TcpClient::sendText(const std::string& text, std::string* response)
{
    /*
     * 连接服务端。
     */
    int fd = SocketUtil::connectToServer(ip_, port_);
    if (fd < 0) {
        return false;
    }

    /*
     * 发送请求文本。
     */
    ssize_t sent = send(fd, text.data(), text.size(), 0);
    if (sent < 0) {
        std::cerr << "[tcp_client] send failed: "
                  << std::strerror(errno) << std::endl;
        SocketUtil::closeSocket(fd);
        return false;
    }

    /*
     * 读取响应。
     */
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    ssize_t n = ::recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (n < 0) {
        std::cerr << "[tcp_client] recv failed: "
                  << std::strerror(errno) << std::endl;
        SocketUtil::closeSocket(fd);
        return false;
    }

    if(n == 0) {
        std::cerr << "[tcp_client] server closed connection" << std::endl;
        SocketUtil::closeSocket(fd);
        return false;
    }

    if(response!= nullptr) {
        *response = std::string(buffer, static_cast<std::size_t>(n));
    }
    SocketUtil::closeSocket(fd);
    return true;
}

bool TcpClient::sendPacket(const Packet& request,Packet* response)
{
     /*
     * 连接服务端。
     */
    int fd = SocketUtil::connectToServer(ip_,port_);
    if(fd < 0)
    {
        return false;
    }

     /*
     * 把 Packet 编码成字节串。
     */
    std::string data = Protocol::encode(request);

     /*
     * 发送完整数据包。
     *
     * 当前包很小，所以一次 send 通常可以发完。
     * 后面上传大文件时，要封装 sendAll，确保所有字节都发送出去。
     */
    if(!sendAll(fd, data.data(), data.size()))
    {
        std::cerr << "[tcp_client] send packet failed: "
                  << std::strerror(errno) << std::endl;
        SocketUtil::closeSocket(fd);
        return false;
    }

    if(response != nullptr)
    {
        if(!recvPacket(fd, response))
        {
            std::cerr << "[tcp_client] recv response packet failed" << std::endl;
            SocketUtil::closeSocket(fd);
            return false;
        }
    }

    SocketUtil::closeSocket(fd);
    return true;
}
