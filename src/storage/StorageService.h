#pragma once

#include "protocol/Packet.h"
#include <string>
#include "storage/Binlog.h"

class StorageService
{
public:
    /*
    * 构造函数。
    *
    * 参数：
    * group_name：当前 storage 所属 group
    * store_path0：真实文件存储目录
    * binlog_path：binlog 文件路径
    */
    StorageService(const std::string& group_name,
                    const std::string& store_path0,
                    const std::string& binlog_path);
                        
    Packet handlePacket(const Packet& request, const std::string& peer_ip);

private:
    Packet handleUploadFile(const Packet& request);
    bool parseUploadBody(const std::string& body,
                         std::string* filename,
                         std::string* content) const;

    /*
    * 处理文件下载请求。
    *
    * request.body 格式：
    * file_id=group1/M00/00/00/xxx.txt
    *
    * 返回：
    * 成功：Status::OK，body 是文件内容
    * 失败：Status::ERROR，body 是错误信息
    */
    Packet handleDownloadFile(const Packet& request);

    /*
    * 处理文件删除请求。
    *
    * request.body 格式：
    * file_id=group1/M00/00/00/xxx.txt
    *
    * 返回：
    * 成功：Status::OK，body 是 delete success
    * 失败：Status::ERROR，body 是错误信息
    */
    Packet handleDeleteFile(const Packet& request);

    /*
    * 处理获取文件元数据请求。
    *
    * request.body 格式：
    * file_id=group1/M00/00/00/xxx.txt
    *
    * 返回：
    * 成功：Status::OK，body 是 metadata 内容
    * 失败：Status::ERROR，body 是错误原因
    */
    Packet handleGetMetadata(const Packet& request);

    /*
    * 处理获取 binlog 请求。
    *
    * request.body 当前可以为空。
    *
    * 返回：
    * 成功：Status::OK，body 是 binlog 文件内容
    * 失败：Status::ERROR
    */
    Packet handleFetchBinlog(const Packet& request);

    /*
    * 处理手动同步请求。
    *
    * request.body 格式：
    * src_ip=127.0.0.1
    * src_port=23000
    *
    * 当前 storage 会作为 dst storage，
    * 主动连接 src storage 拉取 binlog，并根据 binlog 同步文件。
    */
    Packet handleSyncPull(const Packet& request);

    std::string generateFileId(const std::string& filename) const;
    bool writeFile(const std::string& file_path, const std::string& content) const;

    /*
    * 解析下载请求 body。
    *
    * 参数：
    * body：请求体
    * file_id：输出参数
    *
    * 返回：
    * true：解析成功
    * false：解析失败
    */
    bool parseDownloadBody(const std::string& body,
                        std::string* file_id) const;

    /*
    * 从磁盘读取文件内容。
    *
    * 参数：
    * real_path：真实磁盘路径
    * content：输出参数，保存文件内容
    *
    * 返回：
    * true：读取成功
    * false：读取失败
    */
    bool readFile(const std::string& real_path,
                std::string* content) const;

    /*
    * 从磁盘删除文件。
    *
    * 参数：
    * real_path：真实磁盘路径
    *
    * 返回：
    * true：删除成功
    * false：删除失败
    */
    bool deleteRealFile(const std::string& real_path) const;

    /*
    * 根据 file_id 生成 metadata 文件路径。
    *
    * 参数：
    * file_id：逻辑文件 ID
    *
    * 返回：
    * metadata 文件真实路径
    *
    * 示例：
    * file_id:
    * group1/M00/00/00/xxx.txt
    *
    * meta path:
    * ./data/storage1/files/M00/00/00/xxx.txt.meta
    */
    std::string buildMetaPath(const std::string& file_id) const;

    /*
    * 写入 metadata 文件。
    *
    * 参数：
    * meta_path：metadata 文件路径
    * file_id：逻辑文件 ID
    * filename：原始文件名
    * real_path：真实文件路径
    * size：文件大小
    *
    * 返回：
    * true：写入成功
    * false：写入失败
    */
    bool writeMetadata(const std::string& meta_path,
                    const std::string& file_id,
                    const std::string& filename,
                    const std::string& real_path,
                    std::size_t size) const;

    /*
    * 读取 metadata 文件。
    *
    * 参数：
    * meta_path：metadata 文件路径
    * metadata：输出参数，保存 metadata 内容
    *
    * 返回：
    * true：读取成功
    * false：读取失败
    */
    bool readMetadata(const std::string& meta_path,
                    std::string* metadata) const;

    /*
    * 解析 SYNC_PULL 请求体。
    *
    * 参数：
    * body：请求体
    * src_ip：输出参数，源 storage IP
    * src_port：输出参数，源 storage 端口
    *
    * 返回：
    * true：解析成功
    * false：解析失败
    */
    bool parseSyncPullBody(const std::string& body,
                        std::string* src_ip,
                        int* src_port) const;

    /*
    * 从源 storage 拉取 binlog。
    *
    * 参数：
    * src_ip：源 storage IP
    * src_port：源 storage 端口
    * binlog_content：输出参数，保存源 storage 的 binlog
    *
    * 返回：
    * true：拉取成功
    * false：拉取失败
    */
    bool fetchRemoteBinlog(const std::string& src_ip,
                        int src_port,
                        std::string* binlog_content) const;

    /*
    * 根据一整份 binlog 执行同步。
    *
    * 参数：
    * src_ip：源 storage IP
    * src_port：源 storage 端口
    * binlog_content：源 storage binlog 内容
    *
    * 返回：
    * true：同步成功
    * false：同步过程中出现失败
    */
    bool applyBinlogFromSource(const std::string& src_ip,
                            int src_port,
                            const std::string& binlog_content);

    /*
    * 应用一条 CREATE 日志。
    *
    * 参数：
    * src_ip / src_port：源 storage 地址
    * file_id：需要同步的文件 ID
    * filename：原始文件名
    * size：源文件大小，当前只用于日志
    *
    * 返回：
    * true：同步成功
    * false：同步失败
    */
    bool applyCreateFromSource(const std::string& src_ip,
                            int src_port,
                            const std::string& file_id,
                            const std::string& filename,
                            std::size_t size);

    /*
    * 应用一条 DELETE 日志。
    *
    * 参数：
    * file_id：需要删除的文件 ID
    *
    * 返回：
    * true：删除成功或本地不存在
    * false：删除失败
    */
    bool applyDeleteFromSource(const std::string& file_id);
    
private:
    std::string group_name_;
    std::string store_path0_;
    /*
    * storage 操作日志。
    *
    * 上传成功写 CREATE。
    * 删除成功写 DELETE。
    */
    Binlog binlog_;


};
