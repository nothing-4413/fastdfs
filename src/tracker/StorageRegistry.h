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
 * online = true：tracker 认为它在线
 * online = false：tracker 认为它已经下线
 *
 * last_heartbeat 表示最近一次收到心跳的时间。
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
 * tracker 内部的 storage 节点注册表。
 *
 * Step 7 新增：
 * 1. 根据心跳时间标记超时节点
 * 2. 打印当前所有 storage 节点状态
 */
class StorageRegistry
{
public:
     /*
     * 添加或更新 storage 节点。
     *
     * STORAGE_JOIN 时调用。
     */
    void addOrUpdate(const StorageNode& node);

    /*
     * 更新 storage 心跳。
     *
     * 参数：
     * group_name：storage 所属 group
     * ip：storage 的 IP
     * port：storage 的端口
     *
     * 返回：
     * true 表示找到了节点并成功更新
     * false 表示这个 storage 没有注册过
     */
    bool updateHeartbeat(const std::string& group_name, const std::string& ip, int port);

    /*
     * 扫描所有 storage 节点。
     *
     * 如果：
     * 当前时间 - last_heartbeat > timeout_seconds
     *
     * 就把这个节点标记为 offline。
     *
     * 返回值：
     * 本次被标记为 offline 的节点数量。
     */
    int makeTimeoutNodes(int timeout_seconds);

    /*
     * 返回当前所有 storage 节点。
     *
     * 这里返回 vector，是为了后面方便遍历和调试输出。
     */
    std::vector<StorageNode> listALL() const;

    /*
     * 打印所有 storage 节点状态。
     *
     * 这个函数主要用于当前阶段调试。
     * 后面可以替换成管理命令或者 HTTP 管理接口。
     */
    void dumpNodes() const;

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
    static std::string makeKey(const StorageNode& node);

    /*
     * 根据 group_name + ip + port 生成唯一 key。
     *
     * 这个重载给 updateHeartbeat 使用。
     */
    static std::string makeKey(const std::string& group_name, const std::string& ip, int port);

private:
    std::unordered_map<std::string, StorageNode> nodes_;
}

