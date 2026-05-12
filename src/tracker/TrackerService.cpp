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