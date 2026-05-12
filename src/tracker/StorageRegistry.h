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
 * tracker 需要记录：
 * 1. 它属于哪个 group
 * 2. 它的 IP 和端口
 * 3. 它的存储路径
 * 4. 最近一次心跳时间
 * 5. 当前是否在线
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
 * Step 5 中它只支持 addOrUpdate。
 * Step 6 中新增 updateHeartbeat，用来刷新 storage 的心跳时间。
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
     * 返回当前所有 storage 节点。
     *
     * 这里返回 vector，是为了后面方便遍历和调试输出。
     */
    std::vector<StorageNode> listALL() const;

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

