#pragma once

#include "protocol/Command.h"

#include <cstdint>
#include <string>

/*
 * PacketHeader
 *
 * 协议头部。
 *
 * 这个结构对应 FastDFS 官方协议中的 TrackerHeader 思想：
 *
 * body_length：body 的长度
 * cmd：请求/响应命令
 * status：响应状态
 *
 * 注意：
 * 这里不要直接把 struct 原样 send 出去。
 *
 * 原因：
 * 1. struct 可能有内存对齐 padding
 * 2. 不同机器大小端可能不同
 *
 * 所以后面 Protocol.cpp 会手动 encode/decode。
 */
struct PacketHeader
{
    uint64_t body_length;
    Command cmd;
    Status status;
};

/*
 * Packet
 *
 * 一个完整网络数据包。
 *
 * header：固定长度头部
 * body：可变长度数据
 *
 * 举例：
 *
 * PING 请求：
 * header.body_length = 5
 * header.cmd = Command::PING
 * header.status = Status::OK
 * body = "hello"
 */
struct Packet
{
    PacketHeader header;
    std::string body;
};
