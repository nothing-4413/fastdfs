#include "net/TcpClient.h"

#include "net/SocketUtil.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

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