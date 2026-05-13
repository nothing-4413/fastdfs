#include "tracker/TrackerService.h"

#include "protocol/Command.h"
#include "protocol/Protocol.h"

#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

/*
 * parseKeyValueBody
 *
 * 把多行 key=value 字符串解析成 map。
 *
 * 示例：
 *
 * group_name=group1
 * port=23000
 *
 * 解析后：
 *
 * map["group_name"] = "group1"
 * map["port"] = "23000"
 */
static std::unordered_map<std::string,std::string> 
parseKetyValueBody(const std::string& body)
{
    std::unordered_map<std::string,std::string> result;

    std::istringstream input(body);
    std::string line;

    while(std::getline(input,line))
    {
        if(line.empty())
        {
            continue;
        }

        std::size_t pos = line.find('=');
        if(pos == std::string::npos)
        {
            continue;
        }

        std::string key = line.substr(0,pos);
        std::string value = line.substr(pos+1);

        result[key] = value;

    }

    return result;

}

/*
 * buildStorageResponseBody
 *
 * 把 tracker 选中的 storage 节点打包成 key=value 格式。
 *
 * client 收到后会解析出：
 * group_name
 * ip
 * port
 */
static std::string buildStorageResponseBody(const StorgeNode& node)
{
    std::ostringstream oss;

    oss << "group_name=" << node.group_name << "\n";
    oss << "ip=" << node.ip << "\n";
    oss << "port=" << node.port << "\n";

    return oss.str();
}

/*
 * 从 body 中解析 storage 节点信息。
 *
 * body 格式：
 * group_name=group1
 * ip=127.0.0.1
 * port=23000
 */
static bool parseStorageNodeFromBody(
    const std::unordered_map<std::string, std::string>& kv,
    StorageNode* node) {
    if (node == nullptr) {
        return false;
    }

    if (kv.find("group_name") == kv.end() ||
        kv.find("ip") == kv.end() ||
        kv.find("port") == kv.end()) {
        return false;
    }

    node->group_name = kv.at("group_name");
    node->ip = kv.at("ip");

    try {
        node->port = std::stoi(kv.at("port"));
    } catch (...) {
        return false;
    }

    /*
     * 这些字段对 file index 来说不是关键。
     */
    node->base_path = "";
    node->store_path0 = "";
    node->last_heartbeat = 0;
    node->online = true;

    return true;
}

/*
 * 从 file_id 中解析 group_name。
 *
 * file_id:
 * group1/M00/00/00/xxx.txt
 *
 * 返回：
 * group1
 */
static std::string parseGroupFromFileId(const std::string& file_id) {
    std::size_t pos = file_id.find('/');

    if (pos == std::string::npos) {
        return "";
    }

    return file_id.substr(0, pos);
}

Packet TrackerService::handlePacket(const Packet&request,const std::string& peer_ip)
{
     /*
     * 根据请求命令分发。
     *
     * Step 6 新增：
     * Command::STORAGE_HEARTBEAT
     */
    switch(request.header.cmd)
    {
        case Command::PING:
            return handlePing(request);

        case Command::STORAGE_JOIN:
            return handleStorageJoin(request, peer_ip);

        case Command::STORAGE_HEARTBEAT:
            return handleStorageHeartbeat(request, peer_ip);

        case Command::QUERY_UPLOAD_STORAGE:
            return handleQueryUploadStorage(request);

        case Command::QUERY_DOWNLOAD_STORAGE:
            return handleQueryDownloadStorage(request);

        case Command::REPORT_FILE_UPLOAD:
            return handleReportFileUpload(request);

        default:
            return Protocol::makePacket(
                Command::RESPONSE,
                Status::ERROR,
                "unknown command"
            );
    }
}


void TrackerService::checkAlive(int timeout_seconds)
{
     /*
     * 这个函数由后台线程周期性调用。
     *
     * 它不处理网络请求，只检查 registry_ 中已有节点的状态。
     */
    int offline_count = registry_.makeTimeoutNodes(timeout_seconds);

    if(offline_count > 0)
    {
        std::cout << "[tracker] check alive, offline nodes=" << offline_count << std::endl;
    }

    /*
     * 每次检查后打印当前节点状态，方便学习和调试。
     * 后面项目稳定后，可以删除这行或改成 debug 日志。
     */
    registry_.dumpNodes();
}

Packet TrackerService::handlePing(const Packet& request)
{
    (void)request;

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        "PONG from tracker"
    );
}

Packet TrackerService::handleStorageJoin(const Packet& request,const std::string& peer_ip)
{
    Storge node;

    if(!parseStorageJoinRequest(request,node,peer_ip,&node))
    {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "storage join failed: bad body"
        );
    }

    /*
     * 注册或更新 storage 节点。
     */
    registry_.addOrUpdate(node);

    std::cout << "[tracker] storage joined"
              << ", group=" << node.group_name
              << ", ip=" << node.ip
              << ", port=" << node.port
              << ", base_path=" << node.base_path
              << ", store_path0=" << node.store_path0
              << ", total_nodes=" << registry_.size()
              << std::endl;
    
    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        "storage join success"
    );
}

Packet TrackerService::handleStorageHeartbeat(const Packet& request,const std::string& peer_ip)
{
    std::string group_name;
    int port = 0;
    
    if(!parseStorageHeartbeatBodey(request.body,&group_name,&port))
    {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "heartbeat failed: bad body"
        );
    }

    /*
     * 根据 group_name + peer_ip + port 更新心跳。
     *
     * peer_ip 是 TcpServer 从 socket 连接里拿到的对端 IP。
     * port 是 storage 在 body 里主动上报的服务端口。
     */
    bool ok = registry_.updateHeartbeat(group_name, peer_ip, port);

    if(!ok)
    {
        std::cout << "[tracker] heartbeat from unknown storage"
                  << ", group=" << group_name
                  << ", ip=" << peer_ip
                  << ", port=" << port
                  << std::endl;

        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "heartbeat failed: storage not registered"
        );
    }

    std::cout << "[tracker] heartbeat"
              << ", group=" << group_name
              << ", ip=" << peer_ip
              << ", port=" << port
              <<",time=" << std::time(nullptr)
              << std::endl;

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        "heartbeat ok"
    );
}

Packet TrackerService::handleQueryUploadStorage(const Packet& request)
{   
    /*
     * 当前 request.body 可以为空。
     *
     * 后面如果支持指定 group 上传，可以让 body 传：
     *
     * group_name=group1
     */
    std::unordered_map<std::string,std::string> kv = parseKetyValueBody(request.body);

    std::string group_name;

    if(kv.find("group_name") != kv.end())
    {
        group_name = kv["group_name"];
    }

    /*
     * 从 registry_ 里选一个 storage 节点。
     *
     * 目前的选取策略是随机选一个在线的节点，后面可以改成更智能的负载均衡算法。
     */
    StorgeNode selected;
    bool ok = registry_.selectUploadStorge(group_name, &selected);

    if(!ok)
    {
        std::cout << "[tracker] query upload storage failed"
                  << ", no available storage"
                  << ", group=" << group_name
                  << std::endl;

        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "no available storage"
        );
    }

    std::cout << "[tracker] query upload storage"
              << ", selected group=" << selected.group_name
              << ", ip=" << selected.ip
              << ", port=" << selected.port
              << std::endl;

    std::string body = buildStorageResponseBody(selected);

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        body
    );
}

bool TrackerService::parseStorageJoinBody(const std::string& body,const std::string& peer_ip,StorgeNode* node)
{
    if(node == nullptr)
    {
        return false;
    }

    std::unordered_map<std::string,std::string> kv = parseKetyValueBody(body);

    if(kv.find("group_name") == kv.end() ||
       kv.find("port") == kv.end())
    {
        return false;
    }
    

    node->group_name = kv["group_name"];
    node->ip = peer_ip;

    try
    {
        node->port = std::stoi(kv["port"]);   
    }
    catch(...)
    {
        return false;
    }

    if(kv.find("base_path") != kv.end())
    {
        node->base_path = kv["base_path"];
    }

    if(kv.find("store_path0") != kv.end())
    {
        node->store_path0 = kv["store_path0"];
    }

    /*
     * 注册时顺便刷新心跳时间。
     * 后面实现心跳检测时会使用这个字段。
     */
    node->last_heartbeat = std::time(nullptr);
    node->online = true;

    return true;
}

bool TrackerService::parseStorageHeartbeatBodey(const std::string& body,std::string* group_name,int* port)
{
    if(group_name == nullptr || port == nullptr)
    {
        return false;
    }

    std::unordered_map<std::string,std::string> kv = parseKetyValueBody(body);

    if(kv.find("group_name") == kv.end() ||
       kv.find("port") == kv.end())
    {
        return false;
    }

    *group_name = kv["group_name"];

    try
    {
        *port = std::stoi(kv["port"]);   
    }
    catch(...)
    {
        return false;
    }

    return true;
}

Packet TrackerService::handleQueryDownloadStorage(const Packet& request)
{
    /*
     * request.body 格式：
     * file_id=group1/M00/00/00/xxx.txt
     */
    std::unordered_map<std::string, std::string> kv =
        parseKeyValueBody(request.body);

    if (kv.find("file_id") == kv.end()) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "missing file_id"
        );
    }

    std::string file_id = kv["file_id"];
    std::string group_name = parseGroupFromFileId(file_id);

    if (group_name.empty()) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "bad file_id"
        );
    }

    StorageNode selected;

    bool ok = file_index_.get(file_id, &selected);

    if (!ok) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "file index not found"
        );
    }

    if (!registry_.isOnline(selected)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "file storage is offline"
        );
    }

    std::string body = buildStorageResponseBody(selected);

    std::cout << "[tracker] query download storage success"
            << ", file_id=" << file_id
            << ", group=" << selected.group_name
            << ", ip=" << selected.ip
            << ", port=" << selected.port
            << std::endl;

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        body
    );
}

Packet TrackerService::handleReportFileUpload(const Packet& request) {
    std::unordered_map<std::string, std::string> kv =
        parseKeyValueBody(request.body);

    if (kv.find("file_id") == kv.end()) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "missing file_id"
        );
    }

    std::string file_id = kv["file_id"];

    StorageNode node;

    if (!parseStorageNodeFromBody(kv, &node)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "bad storage info"
        );
    }

    /*
     * 确认这个 storage 是 tracker 当前已知且在线的节点。
     * 避免 client 上报一个不存在的 storage。
     */
    if (!registry_.isOnline(node)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "storage not online"
        );
    }

    file_index_.put(file_id, node);

    std::cout << "[tracker] file index added"
              << ", file_id=" << file_id
              << ", group=" << node.group_name
              << ", ip=" << node.ip
              << ", port=" << node.port
              << ", total_index=" << file_index_.size()
              << std::endl;

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        "report file upload success"
    );
}