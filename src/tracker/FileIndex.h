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
    * 从文件加载索引。
    *
    * 参数：
    * filename：索引文件路径，例如 ./data/tracker/file_index.dat
    *
    * 返回：
    * true：加载成功，或者文件不存在但允许继续
    * false：文件存在但读取/解析失败
    */
    bool loadFromFile(const std::string& filename);

    /*
    * 把当前内存索引保存到文件。
    *
    * 参数：
    * filename：索引文件路径
    *
    * 返回：
    * true：保存成功
    * false：保存失败
    */
    bool saveToFile(const std::string& filename) const;

    /*
    * 打印当前文件索引，用于调试。
    */
    void dump() const;

    /*
     * 当前索引数量。
     */
    std::size_t size() const;

private:
    std::unordered_map<std::string, StorageNode> index_;
};