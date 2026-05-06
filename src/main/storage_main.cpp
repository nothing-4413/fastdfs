#include "common/Config.h"
#include "net/TcpClient.h"
#include "protocol/Command.h"
#include "protocol/Protocol.h"

#include <iostream>
#include <sstream>
#include <string>

/*
 * storage_main.cpp
 *
 * Step 5：
 *
 * storage_server 启动后不再只是读取配置。
 * 它会主动连接 tracker_server，并发送 STORAGE_JOIN 注册包。
 */
static bool parseHoppstPort(const std::string& address, std::string* host, int* port)
{
    std::size_t pos = address.find(':');
    if(pos == std::string::npos)
    {
        return false;
    }

    *host = address.substr(0,pos);

    try
    {
      *port = std::stoi(adress.substr(pos + 1));
    }
    catch(...)
    {
      return false;
    }

    return !host->empty() && *port > 0 ;
}

/*
 * buildStorageJoinBody
 *
 * 构造 STORAGE_JOIN 的 body。
 *
 * 当前使用 key=value 多行文本，方便你调试和学习。
 *
 * 后面可以改成更严格的二进制结构。
 */
static std::string buildStorageJoinBody(const std::string& group_name,
                                        int port,
                                        const std::string& base_path,
                                        const std::string& store_path0) 
{
  std::ostringstream oss;

    oss << "group_name=" << group_name << "\n";
    oss << "port=" << port << "\n";
    oss << "base_path=" << base_path << "\n";
    oss << "store_path0=" << store_path0 << "\n";

    return oss.str();
}

 int main(int argc,char* argv[])
 {
    /*
     * storage.conf 里会保存：
     * group_name
     * port
     * base_path
     * store_path0
     * tracker_server
     */

     if(argc < 2)
     {
        std::cerr << "usage: storage_server <storage_conf>" << std::endl;
        return 1;
     }

    /*
     * 保存 storage 配置文件路径。
     * 下一步会实现 Config 模块，然后真正读取里面的 group_name、port 等配置。
     */

    std::string conf_path = argv[1];

   Config config;

   if(!config.load(conf_path))
   {
       std::cerr << "[storage] load config failed" << std::endl;
       return 1;
   }

   /*
     * 当前 storage 所属 group。
     *
     * FastDFS 的扩容和副本同步都围绕 group 展开。
     * 同一个 group 内的 storage 保存相同文件。
     * 不同 group 之间文件相互独立。
     */
   std::string group_name = config.getString("group_name","group1");

   /*
     * storage 对外提供上传、下载服务的端口。
     */
   int port = config.getInt("port",23000);

   /*
     * storage 的工作目录。
     * 后面会保存 binlog、状态文件、日志等。
     */
   std::string base_path = config.getString("base_path","/data/storage1");

   /*
     * 真正保存文件的路径。
     *
     * 后续上传文件时，文件会落到这个目录下面。
     */
   std::string store_path0 = config.getString("store_path0","/data/storage1/files");

    /*
     * tracker 地址。
     *
     * storage 启动后会连接这个地址，并发送 STORAGE_JOIN。
     */
   std::string tracker_server = config.getString("tracker_server","127.0.0.1:22122");

   /*
     * 心跳间隔。
     *
     * storage 会每隔 heart_beat_interval 秒给 tracker 发一次心跳。
     */
   int heart_beat_interval = config.getInt("heart_beat_interval",5);

   /*
     * 磁盘状态上报间隔。
     *
     * 后面会定期上报总空间、剩余空间、文件数等信息。
     */
   int stat_report_interval = config.getInt("stat_report_interval",30);

   std::cout << "[storage] starting..." << std::endl;
   std::cout << "[storage] config: " << conf_path << std::endl;
   std::cout << "[storage] group_name: " << group_name << std::endl;
   std::cout << "[storage] port: " << port << std::endl;
   std::cout << "[storage] base_path: " << base_path << std::endl;
   std::cout << "[storage] store_path0: " << store_path0 << std::endl;
   std::cout << "[storage] tracker_server: " << tracker_server << std::endl;
   std::cout << "[storage] heart_beat_interval: "
              << heart_beat_interval << std::endl;
   std::cout << "[storage] stat_report_interval: "
              << stat_report_interval << std::endl;
   std::cout << "[storage] status: config loaded" << std::endl;

    /*
     * 解析 tracker_server。
     *
     * 例如：
     * 127.0.0.1:22122
     *
     * host = 127.0.0.1
     * tracker_port = 22122
     */
    std::string tracker_host;
    int tracker_port = 0;

    if(!parseHostPort(tracker_server, &tracker_host, &tracker_port))
    {
        std::cerr << "[storage] invalid tracker_server: " << tracker_server << std::endl;
        return 1;
    }

    /*
     * 创建到 tracker 的客户端连接对象。
     */
    TcpClient client(tracker_host, tracker_port);

    /*
     * 构造 STORAGE_JOIN 请求 body。
     */
    std::string body = buildStorageJoinBody(
        group_name,
        port,
        base_path,
        store_path0
    );

    /*
     * 构造 STORAGE_JOIN 包。
     */
    Packet request = Protocol::makePacket(
        Command::STORAGE_JOIN,
        Status::OK,
        body
    );

    Packet response;

    /*
     * 发送注册包给 tracker。
     */
    if(!client.sendPacket(request, &response))
    {
        std::cerr << "[storage] join tracker failed" << std::endl;
        return 1;
    }

    std::cout << "[storage] join response status: "
              << static_cast<int>(response.header.status) << std::endl;
    std::cout << "[storage] join response body: "
              << response.body << std::endl;
              
    /*
     * 当前 Step 5 注册成功后直接退出。
     *
     * 下一步 Step 6 会让 storage 保持运行，并定期发送 heartbeat。
     */
   return 0;
}