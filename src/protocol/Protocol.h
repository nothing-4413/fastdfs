#pragma once

#include "protocol/Packet.h"

#include <string>

/*
 * Protocol
 *
 * 负责 Packet 的编码和解码。
 *
 * 为什么需要 Protocol 层？
 *
 * 因为网络发送的是字节流，不认识 C++ 对象。
 * 我们必须把 Packet 转成 string/bytes 后才能 send。
 * 收到数据后，也必须把 bytes 还原成 Packet。
 *
 * 当前协议格式固定为：
 *
 * 8 字节 body_length
 * 1 字节 cmd
 * 1 字节 status
 * N 字节 body
 *
 * 所以 header 总长度是 10 字节。
 */
namespace Protcool
{
    /*
    * 当前协议头长度：
    *
    * uint64_t body_length = 8 字节
    * uint8_t cmd = 1 字节
    * uint8_t status = 1 字节
    */
    const int HEADER_SIZE = 10;

    /*
    * 创建一个 Packet。
    *
    * 这个函数只是为了减少重复代码。
    */
    Packet makePacket(Command cmd,Status,status,const std::string& body);

    /*
    * 把 Packet 编码成可以通过 socket 发送的字节串。
    */
   std::string encode(const Packet& packet);

    /*
    * 把收到的字节串解码成 Packet。
    *
    * 返回 true 表示解码成功。
    * 返回 false 表示数据不完整或格式错误。
    */
    bool decode(const std::string& data,Packet* packet);
} //namespace Protcool