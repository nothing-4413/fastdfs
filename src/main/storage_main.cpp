#include "common/Config.h"
#include "net/TcpClient.h"
#include "protocol/Command.h"
#include "protocol/Protocol.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread> 
#include "net/TcpServer.h"
#include "storage/StorageService.h"

#include <functional>

/*
 * storage_main.cpp
 *
 * Step 6：
 *
 * storage_server 启动后：
 * 1. 读取 storage.conf
 * 2. 向 tracker 发送 STORAGE_JOIN
 * 3. 注册成功后进入循环
 * 4. 每隔 heart_beat_interval 秒发送 STORAGE_HEARTBEAT
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

/*
 * 构造 STORAGE_HEARTBEAT 请求 body。
 *
 * 心跳不需要重复发送所有信息。
 * 当前只发送 tracker 能识别节点所需的最小信息：
 *
 * group_name + port
 *
 * ip 由 tracker 从 TCP 连接对端地址中获取。
 */
static std::string buildHeartbeatBody(const std::string& group_name,int port)
{
  std::ostringstream oss;

    oss << "group_name=" << group_name << "\n";
    oss << "port=" << port << "\n";

    return oss.str();
}

static bool sendStorageJoin(const std::string& tracker_host, int tracker_port,
                         const std::string& group_name, int storage_port,
                         const std::string& base_path, const std::string& store_path0)
{
    TcpClient client(tracker_host, tracker_port);

    std::string body = buildStorageJoinBody(
        group_name,
        storage_port,
        base_path,
        store_path0
    );

    Packet request = Protocol::makePacket(
        Command::STORAGE_JOIN,
        Status::OK,
        body
    );

    Packet response;

    if(!client.sendPacket(request, &response))
    {
        std::cerr << "[storage] join tracker failed" << std::endl;
        return false;
    }

    std::cout << "[storage] join response status: "
              << static_cast<int>(response.header.status) << std::endl;
    std::cout << "[storage] join response body: "
              << response.body << std::endl;

    return response.header.status == Status::OK;
}

static bool sendHeartbeat(const std::string& tracker_host, int tracker_port,
                          const std::string& group_name, int storage_port)
{
    TcpClient client(tracker_host, tracker_port);

    std::string body = buildHeartbeatBody(group_name, storage_port);

    Packet request = Protocol::makePacket(
        Command::STORAGE_HEARTBEAT,
        Status::OK,
        body
    );

    Packet response;

    if(!client.sendPacket(request, &response))
    {
        std::cerr << "[storage] send heartbeat failed" << std::endl;
        return false;
    }

    std::cout << "[storage] heartbeat response status: "
              << static_cast<int>(response.header.status)
              << ", body: " << response.body << std::endl;

    return response.header.status == Status::OK;
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
     * 第一步：先注册。
     *
     * 如果注册失败，不应该进入心跳循环。
     */
    if(!sendStorageJoin(tracker_host, tracker_port,
                        group_name, port, base_path, store_path0))
      {
          return 1;
      }

    std::cout << "[storage] join tracker success" << std::endl;

    /*
     * 第二步：循环发送心跳。
     *
     * 当前为了学习清晰，使用单线程 while(true)。
     *
     * 后面 storage 需要同时监听客户端上传/下载，
     * 那时会把心跳放到独立线程里。
     */
    std::thread heartbeat_thread([=](){
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(heart_beat_interval));

        bool ok = sendHeartbeat(tracker_host, tracker_port, group_name, port);

        if(!ok)
        {
            std::cerr << "[storage] heartbeat failed" << std::endl;
        }
    }
  });

  heartbeat_thread.detach();

  StorageService storage_service(group_name,store_path0);

  TcpServer server("0.0.0.0", storage_port);

  server.setPacketHandler(
    std::bind(&StorageService::handlePacket,
              &storage_service,
              std::placeholders::_1,
              std::placeholders::_2)
  );

  std::const  << "[storage] start storage tcp server on port "
              << storage_port << std::endl;

  if(!server.start())
  {
      std::cerr << "[storage] start tcp server failed" << std::endl;
      return 1;
  }
    
}