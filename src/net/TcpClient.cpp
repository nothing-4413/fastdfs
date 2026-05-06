#include "net/TcpClient.h"

#include "net/SocketUtil.h"
#include "protocol/Protocol.h"

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
    std::string data = Protcool::encode(request);

     /*
     * 发送完整数据包。
     *
     * 当前包很小，所以一次 send 通常可以发完。
     * 后面上传大文件时，要封装 sendAll，确保所有字节都发送出去。
     */
    ssize_t sent = ::send(fd,data.data(),data.size(),0);
    if(sent < 0)
    {
        std::cerr << "[tcp_client] send packet failed: "
                  << std::strerror(errno) << std::endl;
        SocketUtil::closeSocket(fd);
        return false;
    }

    /*
     * 读取响应。
     *
     * 当前阶段响应包很小，先用 4096 字节缓冲区。
     * 后面会实现 recvHeader + recvBody。
     */
    char buffer[4096];
    std::memset(buffer,0,.sizeof(buffer));

    ssize_t n = ::recv(fd,buffer,sizeof(buffer),0);
    if(n < 0)
    {
        std::cerr << "[tcp_client] recv packet failed: "
                  << std::strerror(errno) << std::endl;
        SocketUtil::closeSocket(fd);
        return false;
    }

    if(n == 0)
    {
        std::cerr << "[tcp_client] server closed connection" << std::endl;
        SocketUtil::closeSocket(fd);
        return false;
    }

    std::string response_data(buffer,static_cast<std::size_t>(n));
    if(response != nullptr)
    {
        if(!Protcool::decode(response_data,response))
        {
            std::cerr << "[tcp_client] decode response failed" << std::endl;
            SocketUtil::closeSocket(fd);
            return false;
        }
    }

    SocketUtil::closeSocket(fd);
    return true;
}