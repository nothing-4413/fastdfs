#include<iostream>
#include<string>

/*
 * tracker_main.cpp
 *
 * 这个文件是 tracker 进程的入口。
 *
 * 在 FastDFS 架构中，tracker 不直接存文件。
 * 它负责：
 * 1. 接收 storage server 的注册
 * 2. 接收 storage server 的心跳
 * 3. 维护 storage server 的在线状态
 * 4. 客户端上传时，为客户端选择一个可用 storage
 * 5. 客户端下载时，告诉客户端应该访问哪个 storage
 *
 * 当前 Step 1 只先让 tracker_server 能启动。
 * 后续我们会逐步把 Config、TcpServer、TrackerService 挂到这里。
 */

int main()
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

    std::cout << "[tracker] starting..." << std::endl;
    std::cout << "[tracker] config: " <<conf_path << std::endl;
    std::cout << "[tracker] role: tracker server" << std::endl;
    std::cout << "[tracker] status: initialized" << std::endl;

    /*
     * 当前版本执行到这里就退出。
     * 后面实现 TcpServer 后，这里会进入事件循环，不会立即退出。
     */
    
    return 0;
}