#include "tracker/StorageRegistry.h"

#include <ctime>
#include <iostream>
#include <sstream>

std::string StorageRegistry::makeKey(const std::string& group_name,
                                     const std::string& ip,
                                     int port) {
    std::ostringstream oss;

    oss << group_name << "@"
        << ip << ":"
        << port;

    return oss.str();
}

/*
 * makeKey
 *
 * tracker 需要区分不同 storage。
 *
 * 同一个 group 里可能有多个 storage。
 * 不同 group 里也可能有相同 IP 但不同端口的 storage。
 *
 * 所以这里用 group_name + ip + port 作为唯一标识。
 */
std::string StorageRegistry::makeKey(const StorageNode& node)
{
    std::ostringstream oss;
    oss << node.group_name << "@" << node.ip << ":" << node.port;
    return oss.str();
}

void StorageRegistry::addOrUpdate(const StorageNode& node)
{
    std::string key = makeKey(node);

    /*
     * unordered_map 的 []：
     * 如果 key 不存在，会插入。
     * 如果 key 已经存在，会覆盖原来的 value。
     */
    nodes_[key] = node;
}

bool StorageRegistry::updateHeartbeat(const std::string& group_name,
                                      const std::string& ip,
                                      int port)
{
    std::string key = makeKey(group_name, ip, port);

    std::unordered_map<std::string, StorageNode>::iterator it = nodes_.find(key);

    if(it == nodes_.end()) {
        /*
         * 没找到说明 storage 还没注册过。
         *
         * 正常流程应该是：
         * STORAGE_JOIN 成功后再 STORAGE_HEARTBEAT。
         */
        return false;
    }

     /*
     * 刷新最近心跳时间。
     */
    it->second.last_heartbeat = std::time(nullptr);

    /*
     * 只要收到心跳，就认为它在线。
     */
    it->second.online = true;

    return true;

}

int StorageRegistry::makeTimeoutNodes(int timeout_seconds)
{
    int offline_count = 0;
    std::time_t now = std::time(nullptr);

    for(std::unordered_map<std::string, StorageNode>::iterator it = nodes_.begin(); it != nodes_.end(); ++it) {
        StorageNode& node = it->second;

        /*
         * 如果节点已经是 offline，就不重复统计。
         */
        if(!node.online) {
            continue;
        }

         /*
         * 判断是否超时。
         *
         * 例如：
         * timeout_seconds = 15
         *
         * 如果当前时间比最近一次心跳晚了超过 15 秒，
         * tracker 就认为 storage 已经失联。
         */
        double diff = std::difftime(now, node.last_heartbeat);

        if(diff > timeout_seconds) {
            node.online = false;
            ++offline_count;

            std::cout << "[tracker] storage offline"
                      << ", group=" << node.group_name
                      << ", ip=" << node.ip
                      << ", port=" << node.port
                      << ", last_heartbeat=" << node.last_heartbeat
                      << ", now=" << now
                      << ", diff=" << diff
                      << std::endl;
        }
    }

    return offline_count;
}

bool StorageRegistry::selectUploadStorage(const std::string& group_name,
                                          StorageNode* selected){
    if(selected == nullptr) {
        return false;
    }

     /*
     * 先收集所有符合条件的 online storage。
     *
     * 为什么不直接在 unordered_map 上轮询？
     * 因为 unordered_map 的遍历顺序不稳定。
     * 先放到 vector 里，逻辑更清晰。
     */
    std::vector<StorageNode> candidates;

    for (std::unordered_map<std::string, StorageNode>::const_iterator it =
             nodes_.begin();
         it != nodes_.end();
         ++it) {
        const StorageNode& node = it->second;

        if (!node.online) {
            continue;
        }

        if (!group_name.empty() && node.group_name != group_name) {
            continue;
        }

        candidates.push_back(node);
    }

    if (candidates.empty()) {
        return false;
    }

    /*
     * 防止 round_robin_index_ 超过当前候选数量。
     *
     * 例如原来有 2 个 storage，现在只剩 1 个 online，
     * 下标需要重新落回合法范围。
     */
    std::size_t index = round_robin_index_ % candidates.size();

    *selected = candidates[index];

    /*
     * 下次选择下一个 storage。
     */
    round_robin_index_ = (round_robin_index_ + 1) % candidates.size();

    std::cout << "[tracker] round robin select"
              << ", index=" << index
              << ", next_index=" << round_robin_index_
              << ", group=" << selected->group_name
              << ", ip=" << selected->ip
              << ", port=" << selected->port
              << std::endl;

    return true;
}

std::vector<StorageNode> StorageRegistry::listALL() const
{
    std::vector<StorageNode> result;

    for(std::unordered_map<std::string, StorageNode>::const_iterator it = nodes_.begin(); it != nodes_.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

void StorageRegistry::dumpNodes() const
{
    std::cout << "[tracker] storage registry dump, total="
              << nodes_.size() << std::endl;

    for(std::unordered_map<std::string, StorageNode>::const_iterator it = nodes_.begin(); it != nodes_.end(); ++it) {
        const StorageNode& node = it->second;

        std::cout << "  - group: " << node.group_name
                  << ", ip: " << node.ip
                  << ", port: " << node.port
                  << ", online: " << (node.online ? "yes" : "no")
                  << ", last_heartbeat: " << node.last_heartbeat
                  << std::endl;
    }
}

std::size_t StorageRegistry::size() const
{
    return nodes_.size();
}

bool StorageRegistry::selectDownloadStorage(const std::string& group_name,StorageNode* selected)const
{
    if (selected == nullptr) {
        return false;
    }

    /*
     * 下载时必须知道 group。
     * 因为 file_id 格式是：
     * group1/M00/00/00/xxx.txt
     */
    if (group_name.empty()) {
        return false;
    }

    /*
     * 当前简化策略：
     * 找到同 group 下第一个 online storage。
     *
     * 后面做副本同步后，同一个 group 内多个 storage 都应该有同一份文件。
     */
    for(std::unordered_map<std::string, StorageNode>::const_iterator it =
             nodes_.begin();
         it != nodes_.end();
         ++it)
    {
        const StorageNode& node = it->second;

        if (!node.online) {
            continue;
        }

        if (node.group_name != group_name) {
            continue;
        }

        *selected = node;
        return true;
    }

    return false;
}

bool StorageRegistry::isOnline(const StorageNode& node) const {
    std::string key = makeKey(node.group_name, node.ip, node.port);

    std::unordered_map<std::string, StorageNode>::const_iterator it =
        nodes_.find(key);

    if (it == nodes_.end()) {
        return false;
    }

    return it->second.online;
}
