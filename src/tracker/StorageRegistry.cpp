#include "tracker/StorageRegistry.h"

#include <sstream>

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
std::string StorageRegistry::makeKey(const StorgeNode& node)
{
    std::ostringstream oss;
    oss << node.group_name << "@" << node.ip << ":" << node.port;
    return oss.str();
}

void StorageRegistry::addOrUpdate(const StorgeNode& node)
{
    std::string key = makeKey(node);

    /*
     * unordered_map 的 []：
     * 如果 key 不存在，会插入。
     * 如果 key 已经存在，会覆盖原来的 value。
     */
    nodes_[key] = node;
}

std::vector<StorgeNode> StorageRegistry::listALL() const
{
    std::vector<StorgeNode> result;

    for(std::unordered_map<std::string, StorgeNode>::const_iterator it = nodes_.begin(); it != nodes_.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

std::size_t StorageRegistry::size() const
{
    return nodes_.size();
}