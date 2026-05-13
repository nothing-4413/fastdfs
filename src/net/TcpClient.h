#pragma once

#include "protocol/Packet.h"

#include <string>

/*
 * TcpClient
 *
 * 最小可用 TCP 客户端。
 *
 * Step 4 开始，TcpClient 不再只发送纯文本。
 * 它可以发送 Packet，并接收 Packet。
 */
class TcpClient
{
public:
    TcpClient(const std::string& ip,int port);

    /*
     * 发送一段文本消息，并读取文本响应。
     *
     * 这个接口暂时保留，方便对比学习。
     */
    bool sendText(const std::string& text, std::string* response);

    /*
     * 发送协议包，并读取响应协议包。
     *
     * 后续所有 FastDFS 风格操作都会用这个接口。
     */
    bool sendPacket(const Packet& packet, Packet* response);

private:
    std::string ip_;
    int port_;
};
