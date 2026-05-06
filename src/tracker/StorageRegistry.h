#pragma once

#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

/*
 * StorageNode
 *
 * 表示一个 storage server 节点。
 *
 * 在 FastDFS 架构中，tracker 要知道有哪些 storage 存在，
 * 每个 storage 属于哪个 group，监听哪个端口，是否在线。
 *
 * 当前阶段先保存最核心的信息：
 * 1. group_name
 * 2. ip
 * 3. port
 * 4. base_path
 * 5. store_path0
 * 6. last_heartbeat
 * 7. online
 */
struct StorageNode {
    std::string group_name;
    std::string ip;
    int port;
    std::string base_path;
    std::string store_path0;
    std::time_t last_heartbeat;
    bool online;
};

/*
 * StorageRegistry
 *
 * tracker 内部维护的 storage 节点注册表。
 *
 * 当前只存在内存里。
 * 后面可以继续扩展：
 * 1. 定期持久化到 data/tracker/storage_status.dat
 * 2. 支持 storage 下线检测
 * 3. 支持按 group 查询 storage
 * 4. 支持负载均衡选择 storage
 */
class StorgeRegistry
{
public:
    /*
     * 添加或更新 storage 节点。
     *
     * 如果这个 storage 是第一次注册，就新增。
     * 如果已经存在，就更新信息和心跳时间。
     */
    void addOrUpdate(const StorgeNode& node);

    /*
     * 返回当前所有 storage 节点。
     *
     * 这里返回 vector，是为了后面方便遍历和调试输出。
     */
    std::vector<StorgeNode> listALL() const;

    /*
     * 返回当前注册节点数量。
     */
    std::size_t size() const;

private:
    /*
     * 生成 storage 节点唯一 key。
     *
     * 当前使用：
     * group_name + ip + port
     *
     * 例如：
     * group1@127.0.0.1:23000
     */
    static std::string makeKey(const StorgeNode& node);

private:
    std::unordered_map<std::string, StorgeNode> nodes_;
}

