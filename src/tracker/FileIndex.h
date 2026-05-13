#pragma once

#include "tracker/StorageRegistry.h"

#include <string>
#include <unordered_map>

/*
 * FileIndex
 *
 * tracker 端的文件位置索引。
 *
 * 作用：
 * 记录 file_id 存在哪个 storage 上。
 *
 * 当前是内存版：
 * file_id -> StorageNode
 *
 * 后面可以扩展：
 * 1. 持久化到本地 index 文件
 * 2. 存到 RocksDB / LevelDB / MongoDB
 * 3. 支持一个 file_id 对应多个 storage 副本
 */
class FileIndex {
public:
    /*
     * 添加或更新文件位置索引。
     *
     * 参数：
     * file_id：逻辑文件 ID，例如 group1/M00/00/00/xxx.txt
     * node：文件所在的 storage 节点
     *
     * 返回：
     * 无
     */
    void put(const std::string& file_id,
             const StorageNode& node);

    /*
     * 根据 file_id 查询文件所在 storage。
     *
     * 参数：
     * file_id：逻辑文件 ID
     * node：输出参数，保存查询到的 storage
     *
     * 返回：
     * true：找到索引
     * false：没有找到索引
     */
    bool get(const std::string& file_id,
             StorageNode* node) const;

    /*
     * 删除文件索引。
     *
     * 参数：
     * file_id：逻辑文件 ID
     *
     * 返回：
     * true：删除成功
     * false：原本就不存在
     */
    bool remove(const std::string& file_id);

    /*
     * 当前索引数量。
     */
    std::size_t size() const;

private:
    std::unordered_map<std::string, StorageNode> index_;
};