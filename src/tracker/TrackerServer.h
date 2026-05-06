#pragma once

#include "protocol/Packet.h"
#include "tracker/StorageRegistry.h"

#include <string>

/*
 * TrackerService
 *
 * tracker 的业务处理层。
 *
 * TcpServer 只负责网络收发。
 * TrackerService 负责根据 cmd 判断业务逻辑。
 *
 * 当前支持：
 * 1. PING
 * 2. STORAGE_JOIN
 *
 * 后续会继续加入：
 * 1. STORAGE_HEARTBEAT
 * 2. QUERY_UPLOAD_STORAGE
 * 3. QUERY_DOWNLOAD_STORAGE
 */
class TrackerService {
public:
    /*
     * 处理一个请求包。
     *
     * request：客户端或 storage 发来的请求
     * peer_ip：对端 IP，由 TcpServer 在 accept 后传入
     *
     * 返回一个响应包。
     */
    Packet handlePacket(const Packet& request, const std::string& peer_ip);

private:
    /*
     * 处理 PING。
     */
    Packet handlePing(const Packet& request);

     /*
     * 处理 storage 注册。
     */
    Packet handleStorageJoin(const Packet& request, const std::string& peer_ip);

    /*
     * 从 STORAGE_JOIN 的 body 中解析 storage 信息。
     *
     * 当前 body 使用简单 key=value 格式：
     *
     * group_name=group1
     * port=23000
     * base_path=./data/storage1
     * store_path0=./data/storage1/files
     */
    bool parseStorageJoinBody(const std::string& body,
                              const std::string& peer_ip,
                              StorageNode* node);

private:
    /*
     * tracker 内部的 storage 注册表。
     */
    StorageRegistry registry_;
};