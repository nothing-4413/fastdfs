#pragma once

#include <string>

/*
 * TcpClient
 *
 * 最小可用 TCP 客户端。
 *
 * 当前作用：
 * fdfs_cli 可以通过它连接 tracker_server。
 *
 * 后面它会继续承担：
 * 1. client 连接 tracker 查询 storage
 * 2. client 连接 storage 上传文件
 * 3. client 连接 storage 下载文件
 * 4. storage 连接 tracker 注册和心跳
 */
class TcpClient
{
public:
    TcpClient(const std::string& ip,int port);

     /*
     * 发送一段文本消息，并读取服务端响应。
     *
     * 当前阶段只用于测试网络是否打通。
     */
    bool sendText(const std::string& text, std::string* response);

private:
    std::string ip_;
    int port_;
};