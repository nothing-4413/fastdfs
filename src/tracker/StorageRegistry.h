#pragma once

#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstddef>

/*
 * StorageNode
 *
 * 表示一个 storage server 节点。
 *
 * tracker 会根据这些信息判断：
 * 1. 这个 storage 属于哪个 groupselectUploadStorage
 * 2. 这个 storage 是否在线
 * 3. client 上传文件时能不能选择它
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
 * tracker 内部维护的 storage 注册表。
 *
 * Step 8 新增：
 * selectUploadStorage()
 *
 * 作用：
 * 从当前 online storage 中选择一个用于上传。
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
    * 使用 Round Robin 选择一个可用于上传的 storage。
    *
    * 参数：
    * group_name：
    *   如果为空，从所有 online storage 中轮询选择。
    *   如果不为空，只从指定 group 的 online storage 中轮询选择。
    *
    * selected：
    *   输出参数，保存被选中的 storage。
    *
    * 返回：
    * true：选择成功
    * false：没有可用 storage
    *
    * 注意：
    * 这个函数会修改 round_robin_index_，所以不能再是 const。
    */
    bool selectUploadStorage(const std::string& group_name,
                            StorageNode* selected);
    
    /*
    * 选择一个可用于下载的 storage。
    *
    * 参数：
    * group_name：从 file_id 中解析出的 group，例如 group1
    * selected：输出参数，保存被选中的 storage
    *
    * 返回：
    * true：找到可用 storage
    * false：没有可用 storage
    *
    * 当前策略：
    * 从指定 group 中选择第一个 online storage。
    */
    bool selectDownloadStorage(const std::string& group_name,
                            StorageNode* selected) const;

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

    /*
    * Round Robin 下标。
    *
    * 每次成功选择一个 storage 后，向后移动。
    */
    std::size_t round_robin_index_ = 0;
}

