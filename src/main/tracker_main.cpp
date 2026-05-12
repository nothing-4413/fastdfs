#include "common/Config.h"
#include "net/TcpServer.h"
#include "tracker/TrackerService.h"

#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

/*
 * tracker_main.cpp
 *
 * Step 7：
 *
 * tracker 启动后台线程，周期性检查 storage 是否超时。
 *
 * 当前超时规则：
 *
 * storage_timeout = check_active_interval * 3
 *
 * 例如：
 * check_active_interval = 10
 * storage_timeout = 30
 *
 * 如果某个 storage 超过 30 秒没有心跳，
 * tracker 就把它标记为 offline。
 */

int main(int argc, char* argv[])
{
    /*
     * argc 表示命令行参数数量。
     * argv 表示命令行参数数组。
     *
     * 例如：
     * ./tracker_server conf/tracker.conf
     *
     * argv[0] = "./tracker_server"
     * argv[1] = "conf/tracker.conf"
     */

    if(argc < 2)
    {
        std::cerr <<"usage: tracker_server <tracker_conf>" << std::endl;
        return 1;
    }

    /*
     * conf_path 保存 tracker 配置文件路径。
     * 现在只是打印出来。
     * 下一步会真正解析这个配置文件。
     */

    std::string conf_path = argv[1];

    /*
     * 创建配置对象。
     */
    Config config;
    
    /*
     * 加载配置文件。
     *
     * 如果失败，说明配置文件路径错误，或者配置文件格式不对。
     */
    if(!config.load(conf_path))
    {
        std::cerr << "[tracker] load config failed" << std::endl;
        return 1;
    }

    /*
     * 从配置中读取 tracker 监听端口。
     *
     * 如果配置文件没有 port，就使用默认值 22122。
     */
    int port = config.getInt("port", 22122);

     /*
     * base_path 是 tracker 的工作目录。
     * 后续 tracker 会在这个目录下保存日志、状态文件等。
     */
    std::string base_path = config.getString("base_path", "/data/tracker");
    
    /*
     * store_lookup 是上传时选择 storage 的策略。
     * 当前只是读取出来，真正负载均衡后面再实现。
     */
    std::string store_lookup = config.getString("store_lookup", "round_robin");

    /*
     * tracker 定期检查 storage 是否还活着。
     * 这个字段就是检查间隔。
     */
    int check_active_interval = config.getInt("check_active_interval", 10);

     /*
     * storage 超时时间。
     *
     * 为了简单，我们先设为 check_active_interval * 3。
     *
     * 也就是说：
     * 如果连续 3 个检查周期都没有心跳，就认为 storage 下线。
     */
    int storage_timeout = check_active_interval * 3;

    std::cout << "[tracker] starting..." << std::endl;
    std::cout << "[tracker] config: " << conf_path << std::endl;
    std::cout << "[tracker] port: " << port << std::endl;
    std::cout << "[tracker] base_path: " << base_path << std::endl;
    std::cout << "[tracker] store_lookup: " << store_lookup << std::endl;
    std::cout << "[tracker] check_active_interval: "
              << check_active_interval << std::endl;
    std::cout << "[tracker] storage_timeout: " << storage_timeout << std::endl;

    /*
     * 创建 tracker 业务服务对象。
     *
     * 注意：
     * service 必须在 server.start() 之前创建。
     * 因为 TcpServer 回调里会使用它。
     */
    TrackerService service;

    /*
     * 启动后台线程做存活检查。
     *
     * 注意：
     * 这里使用引用捕获 [&service]。
     * 因为 service 在 main 函数栈上创建，
     * 后台线程需要访问同一个 service 对象。
     */
    std::thread checker([&service, check_active_interval, storage_timeout](){
        while(true)
        {
            std::this_thread::sleep_for(
                std::chrono::seconds(check_active_interval)
            );

            std::cout << "[tracker] run checkAlive..." << std::endl;

            service.checkAlive(storage_timeout);
        }
    });

    /*
     * detach 表示让这个线程在后台独立运行。
     *
     * 当前项目主线程会进入 server.start() 死循环，
     * 所以 checker 会一直跟着进程运行。
     *
     * 后面做优雅退出时，可以改成 join + stop flag。
     */
    checker.detach();

    /*
     * 创建 TCP 服务端。
     *
     * 0.0.0.0 表示监听本机所有网卡。
     *
     * 如果只想本机访问，可以改成：
     * 127.0.0.1
     */
    TcpServer server("0.0.0.0", port);

    /*
     * 绑定业务回调。
     *
     * std::bind 的含义：
     * 当 TcpServer 收到 Packet 后，调用：
     *
     * service.handlePacket(request, peer_ip)
     *
     * std::placeholders::_1 表示第一个参数 request。
     * std::placeholders::_2 表示第二个参数 peer_ip。
     */
    server.setPacketHandler(
        std::bind(&TrackerService::handlePacket,
                  &service,
                  std::placeholders::_1,
                  std::placeholders::_2)
    );
    
     /*
     * 启动监听。
     *
     * 当前 start() 内部会进入死循环。
     * 所以 tracker_server 会一直运行，不会马上退出。
     */
    if(!server.start())
    {
        std::cerr << "[tracker] tcp server failed" << std::endl;
        return 1;
    }
    
    return 0;
}