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
 * 把这种 body：
 *
 * group_name=group1
 * port=23000
 * base_path=./data/storage1
 *
 * 解析成 unordered_map。
 *
 * 当前这是一个简化版解析函数。
 * 后面如果协议 body 改成 JSON 或二进制结构，这里可以替换。
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
     * 根据 cmd 分发业务。
     *
     * 这一步非常重要：
     * 后面项目功能变多以后，所有请求都会先走到这里。
     */
    switch(request.header.cmd)
    {
        case Command::PING:
            return handlePing(request);

        case Command::STORAGE_JOIN:
            return handleStorageJoin(request, peer_ip);

        default:
            return Protocol::makePacket(
                Command::RESPONSE,
                Status::ERROR,
                "unknown command"
            );
    }
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