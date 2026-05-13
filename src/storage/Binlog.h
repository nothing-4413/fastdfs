#pragma once

#include <cstddef>
#include <string>

/*
 * Binlog
 *
 * storage 端操作日志。
 *
 * 作用：
 * 记录 storage 上发生过的文件变更操作。
 *
 * 当前支持：
 * 1. CREATE：上传文件
 * 2. DELETE：删除文件
 *
 * 后面 storage 副本同步时，会根据 binlog 同步文件。
 */
class Binlog {
public:
    /*
     * 构造函数。
     *
     * 参数：
     * binlog_path：binlog 文件路径
     * 例如：./data/storage1/binlog.dat
     */
    explicit Binlog(const std::string& binlog_path);

    /*
     * 追加 CREATE 记录。
     *
     * 参数：
     * file_id：逻辑文件 ID
     * filename：原始文件名
     * size：文件大小
     *
     * 返回：
     * true：写入成功
     * false：写入失败
     */
    bool appendCreate(const std::string& file_id,
                      const std::string& filename,
                      std::size_t size);

    /*
     * 追加 DELETE 记录。
     *
     * 参数：
     * file_id：逻辑文件 ID
     *
     * 返回：
     * true：写入成功
     * false：写入失败
     */
    bool appendDelete(const std::string& file_id);

private:
    /*
     * 追加一行日志。
     *
     * 参数：
     * line：完整日志行
     *
     * 返回：
     * true：写入成功
     * false：写入失败
     */
    bool appendLine(const std::string& line);

private:
    std::string binlog_path_;
};