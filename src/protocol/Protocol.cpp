#include "protocol/Protocol.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>

namespace{
    /*
    * htonll
    *
    * Linux 标准库提供了：
    * htons：16 位主机字节序转网络字节序
    * htonl：32 位主机字节序转网络字节序
    *
    * 但没有标准的 htonll。
    * 所以我们自己实现 64 位转换。
    *
    * 网络协议中，整数最好统一使用网络字节序，也就是大端序。
    */
    uint64_t htonll(uint64_t value)
    {
        static const int num = 42;

        /*
        * 判断当前机器是不是小端。
        *
        * 如果最低地址的字节是 42，说明小端。
        */
       if(*reinterpret_cast<const char*>(&num) == 42)
       {
        uint32_t high = htonl(static_cast<uint32_t>(value >> 32));
        uint32_t low = htonl(static_cast<uint32_t>(value & 0xFFFFFFFF));
        return (static_cast<uint64_t>(low) << 32) | high;
       }

       return value;
    }

    /*
    * ntohll
    *
    * 网络字节序转主机字节序。
    *
    * 因为 htonll 的转换是对称的，所以这里可以直接复用。
    */
   uint64_t ntohll(uint64_t value)
   {
    return htonll(value);
   }

} //namespace

namespace Protocol
{
    Packet makePacket(Command cmd,Status status,const std::string& body)
    {
        Packet packet;
        packet.header.body_length = body.size();
        packet.header.cmd = cmd;
        packet.header.status = status;
        packet.body = body;
        return packet;
    }

    std::string encode(const Packet& packet)
    {
        /*
        * 最终发送的数据：
        *
        * [0, 7]：body_length
        * [8]：cmd
        * [9]：status
        * [10, ...]：body
        */
       std::string data;
       data.resize(HEADER_SIZE + packet.body.size());

       /*
        * body_length 转成网络字节序后写入前 8 字节。
        */
       uint64_t net_body_length = htonll(packet.header.body_length);
       std::memcpy(&data[0],&net_body_length,sizeof(net_body_length));

       /*
        * cmd 和 status 都是 1 字节，不需要大小端转换。
        */
       data[8] = static_cast<char>(packet.header.cmd);
       data[9] = static_cast<char>(packet.header.status);

       /*
        * 拷贝 body。
        */
       if(!packet.body.empty())
       {
        std::memcpy(&data[HEADER_SIZE],packet.body.data(),packet.body.size());
       }

       return data;
    }

    bool decodeHeader(const std::string& data, PacketHeader* header)
    {
        if(header == nullptr)
        {
            return false;
        }

        /*
        * 至少要有 10 字节 header。
        * 否则连 body_length/cmd/status 都不完整。
        */
       if(data.size() < HEADER_SIZE)
       {
            return false;
       }

       uint64_t net_body_length = 0;
       std::memcpy(&net_body_length,data.data(),sizeof(net_body_length));

       uint64_t body_length = ntohll(net_body_length);

        header->body_length = body_length;
        header->cmd = static_cast<Command>(static_cast<uint8_t>(data[8]));
        header->status = static_cast<Status>(static_cast<uint8_t>(data[9]));

        return true;
    }

    bool decode(const std::string& data,Packet* packet)
    {
        if(packet == nullptr)
        {
            return false;
        }

        PacketHeader header;
        if (!decodeHeader(data, &header))
        {
            return false;
        }

       /*
        * 检查实际收到的数据长度是否等于 header + body。
        *
        * 当前阶段我们一次 recv 假设能收到完整包。
        * 后面实现 Buffer 后，会处理半包和粘包。
        */
        if(header.body_length > data.size() - HEADER_SIZE)
        {
            return false;
        }

        packet->header = header;

        packet->body.assign(data.data() + HEADER_SIZE,
                            static_cast<std::size_t>(header.body_length));

        return true;
    }
} // namespace Protocol
