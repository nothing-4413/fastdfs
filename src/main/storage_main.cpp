#include<iostream>
#include<string>

#include "common/Config.h"

/*
 * storage_main.cpp
 *
 * 这个文件是 storage 进程的入口。
 *
 * 在 FastDFS 架构中，storage 是真正存文件的节点。
 * 它负责：
 * 1. 启动后向 tracker 注册自己
 * 2. 定期向 tracker 发送心跳
 * 3. 接收客户端上传的文件
 * 4. 根据 file_id 返回文件内容
 * 5. 删除文件
 * 6. 后期与同 group 内其他 storage 做副本同步
 *
 * 本阶段新增：
 * 1. 读取 storage.conf
 * 2. 解析 group_name
 * 3. 解析 port
 * 4. 解析 base_path
 * 5. 解析 store_path0
 * 6. 解析 tracker_server
 * 7. 解析 heart_beat_interval
 * 后续 storage 会用这些配置连接 tracker，并启动自己的文件服务端口
 */

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
     * 当前阶段直接退出。
     * Step 3/4 会实现网络连接和 STORAGE_JOIN。
     */ 

   return 0;
}