#include "net/SocketUtil.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace SocketUtil
{
    int createListenSocket(const std::string& ip,int port)
    {
        /*
        * socket()
        *
        * AF_INET：使用 IPv4。
        * SOCK_STREAM：使用 TCP。
        * 0：使用默认协议。
        *
        * 返回值是一个文件描述符 fd。
        */
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0)
        {
            std::cerr << "[socket] socket failed: "
                      << std::strerror(errno) << std::endl;  
            return -1;
        }

        /*
        * SO_REUSEADDR
        *
        * 作用：
        * 程序重启时，可以尽快重新绑定同一个端口。
        *
        * 如果没有这个设置，服务端刚退出后，端口可能处于 TIME_WAIT 状态，
        * 立刻重启会出现 bind failed: Address already in use。
        */
        int opt = 1;
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            std::cerr << "[socket] setsockopt failed: "
                      << std::strerror(errno) << std::endl;  
            close(listen_fd);
            return -1;
        }

        /*
        * sockaddr_in 是 IPv4 地址结构。
        */
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));

         /*
        * inet_pton
        *
        * 把字符串 IP 转成网络字节序的二进制 IP。
        *
        * 例如：
        * "127.0.0.1" -> 4 字节 IPv4 地址
        */
        if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
        {
            std::cerr << "[socket] invalid listen ip: " << ip << std::endl;
            ::close(listen_fd); 
            return -1;
        }

        /*
        * bind()
        *
        * 把 socket 绑定到指定 IP 和端口。
        *
        * 服务端必须 bind。
        * 客户端通常不需要手动 bind。
        */
        if (bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            std::cerr << "[socket] bind failed on "
                      << ip << ":" << port << ", error: "
                      << std::strerror(errno) << std::endl; 
            close(listen_fd);
            return -1;
        }

        /*
        * listen()
        *
        * 把 socket 从普通 fd 变成监听 fd。
        *
        * 第二个参数 backlog 表示等待 accept 的连接队列长度。
        */
        if (listen(listen_fd, 128) < 0)
        {
            std::cerr << "[socket] listen failed: "
                      << std::strerror(errno) << std::endl;  
            ::close(listen_fd);
            return -1;
        }

        return listen_fd;
    }

    int connectToServer(const std::string& ip, int port)
    {
        /*
         *客户端也要先创建 socket。
         */
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
                std::cerr << "[socket] socket failed: "
                        << std::strerror(errno) << std::endl;
                return -1;
        }

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));

        if(inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
        {
                std::cerr << "[socket] invalid server ip: " << ip << std::endl;
                ::close(fd);
                return -1;
        }

        /*
        * connect()
        *
        * 客户端通过 connect 连接服务端。
        *
        * 如果服务端没有启动，通常会失败：
        * Connection refused
        */
        if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        {
                std::cerr << "[socket] connect failed to "
                        << ip << ":" << port << ", error: "
                        << std::strerror(errno) << std::endl;
                ::close(fd);
                return -1;
        }

        return fd;
    }

    void closeSocket(int fd)
    {
        if(fd >= 0)
        {
            ::close(fd);
        }
    }
} // namespace SocketUtil
