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
private:
    std::string group_name_;
    std::string store_path0_;
}