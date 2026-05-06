#include<iostream>
#include<string>

#include "common/Config.h"
#include "net/TcpClient.h"
#include "protocol/Command.h"
#include "protocol/Protocol.h"
/*
 * fdfs_cli.cpp
 *
 * Step 4：
 * ping 命令不再发送普通文本，而是发送协议包 Packet。
 */

static bool parseHostPort(const std::string& address,std::string* host,int* port)
{
     /*
     * 解析 "127.0.0.1:22122" 这种地址。
     */
    std::size_t pos = address.find(':');
    if(pos == std::string::npos)
    {
        return false;
    }

    *host = address.substr(0,pos);

    try
    {
        *port = std::stoi(address.substr(pos + 1));
    }
    catch(...)
    {
        return false;
    }

    return !host->empty() && *port > 0;
}

int main(int argc,char* argv[])
{
    /*
     * 至少需要一个命令参数。
     *
     * 例如：
     * ./fdfs_cli version
     * ./fdfs_cli upload ./test.jpg
     */

    if(argc < 2)
    {
        std::cerr << "usage: fdfs_cli <command> [args]" << std::endl;
        std::cerr << "commands:" << std::endl;
        std::cerr << "  version" << std::endl;
        std::cerr << "  ping" << std::endl;
        std::cerr << "  upload <file>" << std::endl;
        std::cerr << "  download <file_id> <output>" << std::endl;
        return 1;
    }

    /*
     * 客户端配置。
     *
     * 这里默认读取 conf/client.conf。
     * 注意：运行 fdfs_cli 时要在项目根目录下执行。
     */
    Config config;
    if(!config.load("conf/client.conf"))
    {
        std::cerr << "[client] load conf/client.conf failed" << std::endl;
        return 1;
    }
    
    std::string tracker_server = config.getString("tracker_server",
                                                  "127.0.0.1:22122");
    int connect_timeout = config.getInt("connect_timeout", 5);
    int network_timeout = config.getInt("network_timeout", 30);

    /*
     * argv[1] 是用户输入的命令。
     */
    std::string command = argv[1];

    /*
     * version 命令用于验证客户端程序是否能正常运行。
     */
    if(command == "version")
    {
        std::cout << "TinyFastDFS version 0.1.0" << std::endl;
        std::cout << "[client] tracker_server: "
                  << tracker_server << std::endl;
        std::cout << "[client] connect_timeout: "
                  << connect_timeout << std::endl;
        std::cout << "[client] network_timeout: "
                  << network_timeout << std::endl;
        return 0;
    }

    /*
     * ping 命令用于测试 client -> tracker 的 TCP 连接。
     *
     * 这一步很重要：
     * 后面的 upload/download 都建立在这个 TCP 连接能力之上。
     */
    if(command == "ping")
    {
        std::string host;
        int port = 0;

        if(!parseHostPort(tracker_server, &host, &port))
        {
            std::cerr << "[client] invalid tracker_server: "
                      << tracker_server << std::endl;
            return 1;
        }

        TcpClient client(host,port);

        /*
         * 构造 PING 请求包。
         *
         * cmd = PING
         * status = OK
         * body = "hello tracker"
         */
        Packet request = Protcool::makePacket(
            Command::PING,
            Status::OK,
            "hello tracker\n"
        );
        
        Packet response;
        if(!client.sendPacket(request, &response))
        {
            std::cerr << "[client] ping tracker failed" << std::endl;
            return 1;
        }

        std::cout << "[client] response cmd: "
                  << static_cast<int>(response.header.cmd) << std::endl;
        std::cout << "[client] response status: "
                  << static_cast<int>(response.header.status) << std::endl;
        std::cout << "[client] response body: "
                  << response.body << std::endl;

        return 0;
    }

    /*
     * upload 命令当前只做参数检查和打印。
     * 后面会改成：
     * 1. 读取 client.conf
     * 2. 连接 tracker
     * 3. 查询可上传的 storage
     * 4. 连接 storage
     * 5. 发送文件内容
     * 6. 打印返回的 file_id
     */
    if(command == "upload")
    {
        if(argc < 3)
        {
            std::cerr << "usage: fdfs_cli upload <file>" << std::endl;
            return 1;
        }

        std::cout << "[client] tracker_server: "
                  << tracker_server << std::endl;
        std::cout << "[client] upload file: " << argv[2] << std::endl;
        std::cout << "[client] upload is not implemented yet" << std::endl;
        return 0;
    }

    /*
     * download 命令当前只做参数检查和打印。
     * 后面会改成：
     * 1. 根据 file_id 查询 tracker
     * 2. tracker 返回可下载的 storage
     * 3. client 连接 storage
     * 4. storage 返回文件内容
     * 5. client 写入 output 文件
     */
    if (command == "download") {
        if (argc < 4) {
            std::cerr << "usage: fdfs_cli download <file_id> <output>" << std::endl;
            return 1;
        }

        std::cout << "[client] tracker_server: "
                  << tracker_server << std::endl;
        std::cout << "[client] download file_id: " << argv[2] << std::endl;
        std::cout << "[client] output: " << argv[3] << std::endl;
        std::cout << "[client] download is not implemented yet" << std::endl;
    }

    /*
     * 如果命令不属于上面任何一种，说明用户输入了未知命令。
     */
    std::cerr << "unknown command: " << command << std::endl;
    return 1;
}