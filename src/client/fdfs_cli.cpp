#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <unordered_map>

#include "common/Config.h"
#include "net/TcpClient.h"
#include "protocol/Command.h"
#include "protocol/Protocol.h"

/*
 * fdfs_cli.cpp
 *
 * Step 8：
 *
 * upload 命令开始执行上传前查询。
 *
 * 当前流程：
 * fdfs_cli upload test.txt
 *   ↓
 * 连接 tracker
 *   ↓
 * 发送 QUERY_UPLOAD_STORAGE
 *   ↓
 * tracker 返回 group_name + ip + port
 *
 * 注意：
 * 这一节还没有真正连接 storage 上传文件。
 */

struct SelectStorage{
    std::string group_name;
    std::string ip;
    int port;
}

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


/*
 * 解析 key=value 多行 body。
 *
 * tracker 返回的 storage 信息格式：
 *
 * group_name=group1
 * ip=127.0.0.1
 * port=23000
 */
static std::unordered_map<std::string,std::string> parseKeyValueBody(const std::string& body)
{
    std::unordered_map<std::string,std::string> result;
    std::istringstream ss(body);
    std::string line;

    while(std::getline(ss,line))
    {
        if (line.empty()) {
            continue;
        }

        std::size_t pos = line.find('=');
        if(pos == std::string::npos)
        {
            continue;
        }

        std::string key = line.substr(0,pos);
        std::string value = line.substr(pos + 1);

        result[key] = value;
    }

    return result;
}

/*
 * queryUploadStorage
 *
 * 向 tracker 查询一个可用于上传的 storage。
 */
static bool queryUploadStorage(const std::string& tracker_host,
                               int tracker_port,SelectedStorage* selected)
{
    if (selected == nullptr) {
    return false;
    }

    selected->group_name = kv["group_name"];
    selected->ip = kv["ip"];

    try {
            selected->port = std::stoi(kv["port"]);
    } 
    catch (...) {
    std::cerr << "[client] invalid storage port: "
              << kv["port"] << std::endl;
    return false;
    }

    TcpClient client(tracker_host,tracker_port);

    /*
     * 当前不指定 group，因此 body 为空。
     *
     * 后面如果要指定 group，可以传：
     * group_name=group1
     */
    Packet request = Protcool::makePacket(
        Command::QUERY_UPLOAD_STORAGE,
        Status::OK,
        ""
    );

    Packet response;

    if(!client.sendPacket(request, &response))
    {
        std::cerr << "[client] query upload storage failed" << std::endl;
        return false;
    }

   if (response.header.status != Status::OK) {
        std::cerr << "[client] tracker returned error: "
                  << response.body << std::endl;
        return false;
    }

    std::unordered_map<std::string,std::string> kv =
        parseKeyValueBody(response.body);

    if (kv.find("group_name") == kv.end() ||
        kv.find("ip") == kv.end() ||
        kv.find("port") == kv.end()) {
        std::cerr << "[client] bad tracker response body: "
                  << response.body << std::endl;
        return false;
    }

    std::cout << "[client] selected storage:" << std::endl;
    std::cout << "  group_name: " << kv["group_name"] << std::endl;
    std::cout << "  ip: " << kv["ip"] << std::endl;
    std::cout << "  port: " << kv["port"] << std::endl;

    return true;
}

static bool readFileContent(const std::string& filename,std::string* content)
{
    if(content == nullptr) {
        return false;
    }

    std::ifstream input(filename.c_str(),std::ios::binary);
    if(!input.is_open()) {
        std::cerr << "[client] open local file failed: "
                  << filename << std::endl;
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    *content = buffer.str();
    return true;
}

static std::string basenameOf(const std::string& path)
{
    std::size_t pos = path.find_last_of('/');

    if (pos == std::string::npos) {
        return path;
    }

    return path.substr(pos + 1);
}

static bool uploadFIleToStorage(const SelectedStorage& storage,const std::string& local_file)
{
    std::String content;

    if(!readFileContent(local_file, &content)) {
        return false;
    }

    std::string filename = basenameOf(local_file);

    std::ostringstream body;
    body << "filename=" << filename << "\n";
    body << "content=" << content;

    TcpClient client(storage.ip,storage.port);

    Packet request = Protocol::makePacket(
        Command::UPLOAD_FILE,
        Status::OK,
        body.str()
    );

    Packet response;

     if (!client.sendPacket(request, &response)) {
        std::cerr << "[client] upload file to storage failed"
                  << std::endl;
        return false;
    }

    if (response.header.status != Status::OK) {
        std::cerr << "[client] storage returned error: "
                  << response.body << std::endl;
        return false;
    }

    std::cout << "[client] upload success" << std::endl;
    std::cout << response.body;

    return true;

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
        std::cerr << "  delete <file_id>" << std::endl;
        std::cerr << "  stat <file_id>" << std::endl;
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

    std::string tracker_host;
    int tracker_port = 0;

    if(!parseHostPort(tracker_server, &tracker_host, &tracker_port))
    {
        std::cerr << "[client] invalid tracker_server: "
                  << tracker_server << std::endl;
        return 1;
    }

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
        TcpClient client(tracker_host,tracker_port);

        Packet request = Protcool::makePacket(
            Command::PING,
            Status::OK,
            "hello tracker"
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

        std::string filename = argv[2];

        std::cout << "[client] upload file: "
                  << filename << std::endl;
        
        SelectedStorage selected;


        /*
         * 先查询 tracker。
         *
         * 注意：
         * 当前只是查询 storage，不读取文件内容。
         */
        if(!queryUploadStorage(tracker_host, tracker_port,&selected))
        {
            return 1;
        }

        std::cout << "[client] selected storage:" << std::endl;
        std::cout << "  group_name: " << selected.group_name << std::endl;
        std::cout << "  ip: " << selected.ip << std::endl;
        std::cout << "  port: " << selected.port << std::endl;
                
        if(!uploadFileToStorage(selected, filename)) {
        return 1;
        }

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

        std::string file_id = argv[2];
        std::string output = argv[3];

        std::cout << "[client] download file_id: "
                << file_id << std::endl;
        std::cout << "[client] output: "
                << output << std::endl;

        SelectedStorage selected;

        if (!queryDownloadStorage(tracker_host,
                                tracker_port,
                                file_id,
                                &selected)) {
            return 1;
        }

        std::cout << "[client] selected storage:" << std::endl;
        std::cout << "  group_name: " << selected.group_name << std::endl;
        std::cout << "  ip: " << selected.ip << std::endl;
        std::cout << "  port: " << selected.port << std::endl;

        if (!downloadFileFromStorage(selected, file_id, output)) {
            return 1;
        }

        return 0;
    }

    if (command == "delete") {
        if (argc < 3) {
            std::cerr << "usage: fdfs_cli delete <file_id>"
                    << std::endl;
            return 1;
        }

        std::string file_id = argv[2];

        std::cout << "[client] delete file_id: "
                << file_id << std::endl;

        /*
        * 删除前也要先问 tracker。
        *
        * 当前复用 queryDownloadStorage：
        * 因为 delete 和 download 一样，都是根据 file_id 找到对应 group 的 online storage。
        */
        SelectedStorage selected;

        if (!queryDownloadStorage(tracker_host,
                                tracker_port,
                                file_id,
                                &selected)) {
            return 1;
        }

        std::cout << "[client] selected storage:" << std::endl;
        std::cout << "  group_name: " << selected.group_name << std::endl;
        std::cout << "  ip: " << selected.ip << std::endl;
        std::cout << "  port: " << selected.port << std::endl;

        if (!deleteFileFromStorage(selected, file_id)) {
            return 1;
        }

        return 0;
    }

    if (command == "stat") {
        if (argc < 3) {
            std::cerr << "usage: fdfs_cli stat <file_id>"
                    << std::endl;
            return 1;
        }

        std::string file_id = argv[2];

        std::cout << "[client] stat file_id: "
                << file_id << std::endl;

        /*
        * stat 和 download/delete 一样，都需要先根据 file_id 找 storage。
        */
        SelectedStorage selected;

        if (!queryDownloadStorage(tracker_host,
                                tracker_port,
                                file_id,
                                &selected)) {
            return 1;
        }

        std::cout << "[client] selected storage:" << std::endl;
        std::cout << "  group_name: " << selected.group_name << std::endl;
        std::cout << "  ip: " << selected.ip << std::endl;
        std::cout << "  port: " << selected.port << std::endl;

        if (!getMetadataFromStorage(selected, file_id)) {
            return 1;
        }

        return 0;
    }

    /*
     * 如果命令不属于上面任何一种，说明用户输入了未知命令。
     */
    std::cerr << "unknown command: " << command << std::endl;
    return 1;
}

/*
 * 向 tracker 查询下载 file_id 对应的 storage。
 *
 * 参数：
 * tracker_host：tracker IP
 * tracker_port：tracker 端口
 * file_id：要下载的文件 ID
 * selected：输出参数，保存 tracker 返回的 storage
 *
 * 返回：
 * true：查询成功
 * false：查询失败
 */
static bool queryDownloadStorage(const std::string& tracker_host,
                                 int tracker_port,
                                 const std::string& file_id,
                                 SelectedStorage* selected) {
    if (selected == nullptr) {
        return false;
    }

    TcpClient client(tracker_host, tracker_port);

    std::string body = "file_id=" + file_id;

    Packet request = Protocol::makePacket(
        Command::QUERY_DOWNLOAD_STORAGE,
        Status::OK,
        body
    );

    Packet response;

    if (!client.sendPacket(request, &response)) {
        std::cerr << "[client] query download storage failed"
                  << std::endl;
        return false;
    }

    if (response.header.status != Status::OK) {
        std::cerr << "[client] tracker returned error: "
                  << response.body << std::endl;
        return false;
    }

    std::unordered_map<std::string, std::string> kv =
        parseKeyValueBody(response.body);

    if (kv.find("group_name") == kv.end() ||
        kv.find("ip") == kv.end() ||
        kv.find("port") == kv.end()) {
        std::cerr << "[client] bad tracker response body: "
                  << response.body << std::endl;
        return false;
    }

    selected->group_name = kv["group_name"];
    selected->ip = kv["ip"];

    try {
        selected->port = std::stoi(kv["port"]);
    } catch (...) {
        std::cerr << "[client] invalid storage port: "
                  << kv["port"] << std::endl;
        return false;
    }

    return true;
}

/*
 * 把下载到的内容写入本地文件。
 *
 * 参数：
 * output：输出文件路径
 * content：文件内容
 *
 * 返回：
 * true：写入成功
 * false：写入失败
 */
static bool writeLocalFile(const std::string& output,
                           const std::string& content) {
    std::ofstream file(output.c_str(), std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "[client] open output file failed: "
                  << output << std::endl;
        return false;
    }

    file.write(content.data(),
               static_cast<std::streamsize>(content.size()));

    return file.good();
}

/*
 * 从 storage 下载文件。
 *
 * 参数：
 * storage：tracker 返回的 storage 节点
 * file_id：文件 ID
 * output：本地输出路径
 *
 * 返回：
 * true：下载成功
 * false：下载失败
 */
static bool downloadFileFromStorage(const SelectedStorage& storage,
                                    const std::string& file_id,
                                    const std::string& output) {
    TcpClient client(storage.ip, storage.port);

    std::string body = "file_id=" + file_id;

    Packet request = Protocol::makePacket(
        Command::DOWNLOAD_FILE,
        Status::OK,
        body
    );

    Packet response;

    if (!client.sendPacket(request, &response)) {
        std::cerr << "[client] download from storage failed"
                  << std::endl;
        return false;
    }

    if (response.header.status != Status::OK) {
        std::cerr << "[client] storage returned error: "
                  << response.body << std::endl;
        return false;
    }

    if (!writeLocalFile(output, response.body)) {
        return false;
    }

    std::cout << "[client] download success" << std::endl;
    std::cout << "[client] output: " << output << std::endl;
    std::cout << "[client] size: "
              << response.body.size() << std::endl;

    return true;
}

/*
 * 从 storage 删除文件。
 *
 * 参数：
 * storage：tracker 返回的 storage 节点
 * file_id：要删除的文件 ID
 *
 * 返回：
 * true：删除成功
 * false：删除失败
 */
static bool deleteFileFromStorage(const SelectedStorage& storage,
                                  const std::string& file_id) {
    TcpClient client(storage.ip, storage.port);

    std::string body = "file_id=" + file_id;

    Packet request = Protocol::makePacket(
        Command::DELETE_FILE,
        Status::OK,
        body
    );

    Packet response;

    if (!client.sendPacket(request, &response)) {
        std::cerr << "[client] delete from storage failed"
                  << std::endl;
        return false;
    }

    if (response.header.status != Status::OK) {
        std::cerr << "[client] storage returned error: "
                  << response.body << std::endl;
        return false;
    }

    std::cout << "[client] delete success" << std::endl;
    std::cout << "[client] response: "
              << response.body << std::endl;

    return true;
}

/*
 * 从 storage 获取文件元数据。
 *
 * 参数：
 * storage：tracker 返回的 storage 节点
 * file_id：文件 ID
 *
 * 返回：
 * true：获取成功
 * false：获取失败
 */
static bool getMetadataFromStorage(const SelectedStorage& storage,
                                   const std::string& file_id) {
    TcpClient client(storage.ip, storage.port);

    std::string body = "file_id=" + file_id;

    Packet request = Protocol::makePacket(
        Command::GET_METADATA,
        Status::OK,
        body
    );

    Packet response;

    if (!client.sendPacket(request, &response)) {
        std::cerr << "[client] get metadata from storage failed"
                  << std::endl;
        return false;
    }

    if (response.header.status != Status::OK) {
        std::cerr << "[client] storage returned error: "
                  << response.body << std::endl;
        return false;
    }

    std::cout << "[client] file metadata:" << std::endl;
    std::cout << response.body;

    return true;
}