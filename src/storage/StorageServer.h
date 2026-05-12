#include "protocol/Packet.h"
#include <string>

class StorageServer
{
public:
    StorageServer(const std::string& group_name,
                        std::string store_path0);
                        
    Packet handlePacket(const Packet& request, const std::string& peer_ip);

private:
    Packet handleUploadFile(const Packet& request, const std::string& peer_ip);

    std::string generateFileId(const std::string& filename) const;
    bool writeFile(const std::string& file_path, const std::string& content) const;

private:
    std::string group_name_;
    std::string store_path0_;
}