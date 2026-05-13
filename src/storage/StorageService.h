#include "protocol/Packet.h"
#include <string>

class StorageServer
{
public:
    StorageServer(const std::string& group_name,
                        std::string store_path0);
                        
    Packet handlePacket(const Packet& request, const std::string& peer_ip);

private:
    Packet handleUploadFile(const Packet& request, const std::string& peer_ip);

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
    
private:
    std::string group_name_;
    std::string store_path0_;
}