#pragma once

#include "protocol/Packet.h"
#include "tracker/StorageRegistry.h"
#include "tracker/FileIndex.h"

#include <string>

/*
 * TrackerService
 *
 * tracker 的业务处理层。
 *
 * 当前支持：
 * 1. PING
 * 2. STORAGE_JOIN
 * 3. STORAGE_HEARTBEAT
 * 4. QUERY_UPLOAD_STORAGE
 */
class TrackerService {
public:

    /*
    * 构造函数。
    *
    * 参数：
    * file_index_path：文件索引持久化路径
    */
    explicit TrackerService(const std::string& file_index_path);

    /*
     * 处理一个请求包。
     *
     * request：客户端或 storage 发来的请求
     * peer_ip：对端 IP，由 TcpServer 在 accept 后传入
     *
     * 返回一个响应包。
     */
    Packet handlePacket(const Packet& request, const std::string& peer_ip);

    /*
     * 检查 storage 存活状态。
     *
     * timeout_seconds：
     * 如果某个 storage 超过 timeout_seconds 秒没有心跳，
     * 就将它标记为 offline。
     */
    void checkAlive(int timeout_seconds);

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
     * 处理 storage 心跳。
     */
    Packet handleStorageHeartbeat(const Packet& request, const std::string& peer_ip);
    
    /*
     * 处理客户端上传前的 storage 查询。
     *
     * client 并不直接知道该上传到哪个 storage。
     * 它必须先问 tracker。
     */
    Packet handleQueryUploadStorage(const Packet& request);

    /*
    * 处理客户端下载前的 storage 查询。
    *
    * request.body 格式：
    * file_id=group1/M00/00/00/xxx.txt
    *
    * 返回：
    * 成功：group_name + ip + port
    * 失败：错误原因
    */
    Packet handleQueryDownloadStorage(const Packet& request);

    /*
    * 处理 client 上传成功后的文件索引上报。
    *
    * request.body 格式：
    * file_id=group1/M00/00/00/xxx.txt
    * group_name=group1
    * ip=127.0.0.1
    * port=23000
    *
    * 返回：
    * 成功：Status::OK
    * 失败：Status::ERROR
    */
    Packet handleReportFileUpload(const Packet& request);

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

     /*
     * 解析 STORAGE_HEARTBEAT 的 body。
     *
     * 当前 heartbeat body 格式：
     *
     * group_name=group1
     * port=23000
     */
    bool parseStorageHeartbeatBody(const std::string& body,
                                  std::string* group_name,
                                  int* port);
    
private:
    /*
     * tracker 内部的 storage 注册表。
     */
    StorageRegistry registry_;

    /*
    * 文件位置索引：
    * file_id -> storage
    */
    FileIndex file_index_;

    /*
    * 文件索引持久化路径。
    *
    * 例如：
    * ./data/tracker/file_index.dat
    */
    std::string file_index_path_;

    /*
    * 保存文件索引。
    *
    * 返回：
    * true：保存成功
    * false：保存失败
    */
    bool saveFileIndex();
};