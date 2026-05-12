#include "tracker/StorageRegistry.h"

#include <sstream>
#include <ctime>

std::string StorageRegistry::makeKey(const StorageNode& node) {
    return makeKey(node.group_name, node.ip, node.port);
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

bool StorageRegistry::updateHeartbeat(const std::string& group_name,const std::string& ip.int port)
{
    std::string key = makeKey(group_name,iup,port);

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

std::vector<StorageNode> StorageRegistry::listALL() const
{
    std::vector<StorageNode> result;

    for(std::unordered_map<std::string, StorageNode>::const_iterator it = nodes_.begin(); it != nodes_.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::size_t StorageRegistry::size() const
{
    return nodes_.size();
}