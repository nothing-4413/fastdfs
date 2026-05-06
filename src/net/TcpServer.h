#pragma once

#include "protocol/Packet.h"

#include <functional>
#include <string>

/*
 * TcpServer
 *
 * Step 5 修改：
 *
 * TcpServer 不再自己决定如何处理 cmd。
 * 它只负责：
 * 1. accept 连接
 * 2. recv 数据
 * 3. decode Packet
 * 4. 调用 packet_handler_
 * 5. encode response
 * 6. send response
 *
 * 真正业务逻辑交给 TrackerService。
 */
class TcpServer {
public:
     /*
     * PacketHandler 是一个函数类型。
     *
     * 参数：
     * const Packet&：请求包
     * const std::string&：对端 IP
     *
     * 返回：
     * Packet 响应包
     */
    typedef std::function<Packet(const Packet&,const std::string&)> PacketHandler;
public:
    /*
     * 构造函数。
     *
     * ip：监听地址
     * port：监听端口
     */
    TcpServer(const std::string& ip, int port);

    /*
     * 设置业务处理函数。
     *
     * tracker_server 会把 TrackerService::handlePacket 绑定进来。
     */
    void setPacketHandler(PacketHandler handler) {

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
    void handleClient(int client_fd, const std::string& peer_ip);

private:
    std::string ip_;
    int port_;
    PacketHandler packet_handler_;
};