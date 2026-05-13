#include "storage/StorageService.h"

#include "protocol/Command.h"
#include "protocol/Protocol.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include "net/TcpClient.h"

#include <atomic>
#include <cstdlib>
#include <vector>

static std::vector<std::string> splitByTab(const std::string& line) {
    std::vector<std::string> result;

    std::istringstream input(line);
    std::string item;

    while (std::getline(input, item, '\t')) {
        result.push_back(item);
    }

    return result;
}

static void ensureDir(const std::string& dir) {
    std::string current;

    for (std::size_t i = 0; i < dir.size(); ++i) {
        char c = dir[i];
        current.push_back(c);
        if (c == '/') {
            mkdir(current.c_str(), 0755);
        }
    }

    if (!current.empty()) {
        mkdir(current.c_str(), 0755);
    }
}

static bool containsUnsafePathPart(const std::string& value) {
    return value.empty() ||
           value.find("..") != std::string::npos ||
           value.find('\\') != std::string::npos ||
           value[0] == '/';
}

static bool isSafeFilename(const std::string& filename) {
    return !containsUnsafePathPart(filename) &&
           filename.find('/') == std::string::npos;
}

static bool isSafeFileId(const std::string& file_id,
                         const std::string& group_name) {
    const std::string prefix = group_name + "/M00/00/00/";

    return !containsUnsafePathPart(file_id) &&
           file_id.find(prefix) == 0 &&
           file_id.size() > prefix.size();
}

StorageService::StorageService(const std::string& group_name,
                               const std::string& store_path0,
                               const std::string& binlog_path)
    : group_name_(group_name),
      store_path0_(store_path0),
      binlog_(binlog_path) {
}

Packet StorageService::handlePacket(const Packet& request,
                                    const std::string& peer_ip)
{
    (void)peer_ip; // Unused parameter

    if(request.header.cmd == Command::UPLOAD_FILE) {
        return handleUploadFile(request);
    } 

    if (request.header.cmd == Command::DOWNLOAD_FILE) {
        return handleDownloadFile(request);
    }

    if (request.header.cmd == Command::DELETE_FILE) {
        return handleDeleteFile(request);
    }   

    if (request.header.cmd == Command::GET_METADATA) {
        return handleGetMetadata(request);
    }

    if (request.header.cmd == Command::FETCH_BINLOG) {
        return handleFetchBinlog(request);
    }

    if (request.header.cmd == Command::SYNC_PULL) {
        return handleSyncPull(request);
    }

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::ERROR,
        "unknown storage command"
    );
}

bool StorageService::parseUploadBody(const std::string& body,
                                     std::string* filename,
                                     std::string* content) const
{
    if(filename == nullptr || content == nullptr) {
        return false;
    }

    const std::string filename_key = "filename=";
    const std::string content_key = "\ncontent=";

   if(body.find(filename_key) != 0) {
        return false;
    }

    std::size_t content_pos = body.find(content_key);
    if(content_pos == std::string::npos) {
        return false;
    }

    *filename = body.substr(filename_key.size(), content_pos - filename_key.size());
    *content = body.substr(content_pos + content_key.size());

    return isSafeFilename(*filename);
}

std::string StorageService::generateFileId(const std::string& filename) const
{
    static std::atomic<unsigned long long> sequence(0);

    std::ostringstream oss;

    oss << group_name_
        << "/M00/00/00/"
        << std::time(nullptr)
        << "_"
        << sequence.fetch_add(1)
        << "_"
        << filename;

    return oss.str();
}

std::string StorageService::buildRealPath(const std::string& file_id) const{
    std::string prefix = group_name_+"/";

    std::string relative_path = file_id;

    if(relative_path.find(prefix)==0)
    {
        relative_path = relative_path.substr(prefix.size());
    }

    return store_path0_+"/" + relative_path;
}

bool StorageService::writeFile(const std::string& real_path,const std::string& content)const
{
     /*
     * 当前阶段固定创建 M00/00/00 目录。
     * 后面文件分布策略会改成动态目录。
     */
    ensureDir(store_path0_ + "/M00/00/00");

    std::ofstream output(real_path.c_str(),std::ios::binary);
    if(!output.is_open())
    {
        std::cerr <<"[storage] open file failed: "
                  <<real_path <<std::endl;
        return false;
    }

    output.write(content.data(),static_cast<std::streamsize>(content.size()));

    return output.good();
}

Packet StorageService::handleUploadFile(const Packet& request)
{
    std::string filename;
    std::string content;

    if(!parseUploadBody(request.body,&filename,&content))
    {
        return Protocol::makePacket(
               Command::RESPONSE,
               Status::ERROR,
               "bad upload body"
        );
    }

    std::string file_id = generateFileId(filename);
    std::string real_path = buildRealPath(file_id);

    if(!writeFile(real_path,content))
    {
        return  Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "write file failed"
        );
    }

    std::string meta_path = buildMetaPath(file_id);

    if (!writeMetadata(meta_path,
                    file_id,
                    filename,
                    real_path,
                    content.size())) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "write metadata failed"
        );
    }

    /*
    * 文件和 metadata 都写成功后，记录 CREATE binlog。
    */
    if (!binlog_.appendCreate(file_id, filename, content.size())) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "write create binlog failed"
        );
    }

    std::cout << "[storage] upload success"
          << ", filename=" << filename
          << ", file_id=" << file_id
          << ", real_path=" << real_path
          << ", meta_path=" << meta_path
          << ", size=" << content.size()
          << std::endl;

    std::string response_body = "file_id=" + file_id + "\n";

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        response_body
    );
}

bool StorageService::parseDownloadBody(const std::string& body,
                                       std::string* file_id) const 
{
    if (file_id == nullptr) {
        return false;
    }

    const std::string key = "file_id=";

    if (body.find(key) != 0) {
        return false;
    }

    *file_id = body.substr(key.size());

    return isSafeFileId(*file_id, group_name_);
}

bool StorageService::readFile(const std::string& real_path,
                              std::string* content) const {
    if (content == nullptr) {
        return false;
    }

    std::ifstream input(real_path.c_str(), std::ios::binary);
    if (!input.is_open()) {
        std::cerr << "[storage] open file for read failed: "
                  << real_path << std::endl;
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    *content = buffer.str();

    return true;
}

Packet StorageService::handleDownloadFile(const Packet& request) {
    std::string file_id;

    if (!parseDownloadBody(request.body, &file_id)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "bad download body"
        );
    }

    std::string real_path = buildRealPath(file_id);

    std::string content;

    if (!readFile(real_path, &content)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "read file failed"
        );
    }

    std::cout << "[storage] download success"
              << ", file_id=" << file_id
              << ", real_path=" << real_path
              << ", size=" << content.size()
              << std::endl;

    /*
     * 当前学习版直接把文件内容放在 response.body。
     *
     * 后面支持大文件时，需要改成分块发送。
     */
    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        content
    );
}

bool StorageService::deleteRealFile(const std::string& real_path) const {
    /*
     * std::remove 返回 0 表示删除成功。
     * 非 0 表示删除失败。
     *
     * 常见失败原因：
     * 1. 文件不存在
     * 2. 权限不足
     * 3. 路径错误
     */
    if (std::remove(real_path.c_str()) != 0) {
        std::cerr << "[storage] remove file failed: "
                  << real_path << std::endl;
        return false;
    }

    return true;
}

Packet StorageService::handleDeleteFile(const Packet& request) {
    std::string file_id;

    /*
     * 删除请求 body 和下载请求 body 一样：
     *
     * file_id=group1/M00/00/00/xxx.txt
     *
     * 所以这里复用 parseDownloadBody。
     */
    if (!parseDownloadBody(request.body, &file_id)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "bad delete body"
        );
    }

    std::string real_path = buildRealPath(file_id);

    if (!deleteRealFile(real_path)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "delete file failed"
        );
    }

    std::string meta_path = buildMetaPath(file_id);
    if (std::remove(meta_path.c_str()) != 0) {
        std::cerr << "[storage] warning: remove metadata failed or not exists: "
                  << meta_path << std::endl;
    }

    /*
    * 真实文件删除成功后，记录 DELETE binlog。
    */
    if (!binlog_.appendDelete(file_id)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "write delete binlog failed"
        );
    }

    std::cout << "[storage] delete success"
              << ", file_id=" << file_id
              << ", real_path=" << real_path
              << std::endl;

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        "delete success"
    );
}

std::string StorageService::buildMetaPath(const std::string& file_id) const {
    /*
     * 当前设计：
     * metadata 文件和真实文件放在同一目录下，后缀加 .meta。
     *
     * real file:
     * ./data/storage1/files/M00/00/00/xxx.txt
     *
     * meta file:
     * ./data/storage1/files/M00/00/00/xxx.txt.meta
     */
    return buildRealPath(file_id) + ".meta";
}

bool StorageService::writeMetadata(const std::string& meta_path,
                                   const std::string& file_id,
                                   const std::string& filename,
                                   const std::string& real_path,
                                   std::size_t size) const {
    std::ofstream output(meta_path.c_str(), std::ios::binary);

    if (!output.is_open()) {
        std::cerr << "[storage] open metadata file failed: "
                  << meta_path << std::endl;
        return false;
    }

    /*
     * metadata 当前使用 key=value 格式，方便学习和调试。
     */
    output << "file_id=" << file_id << "\n";
    output << "filename=" << filename << "\n";
    output << "size=" << size << "\n";
    output << "create_time=" << std::time(nullptr) << "\n";
    output << "real_path=" << real_path << "\n";

    return output.good();
}

bool StorageService::readMetadata(const std::string& meta_path,
                                  std::string* metadata) const {
    if (metadata == nullptr) {
        return false;
    }

    std::ifstream input(meta_path.c_str(), std::ios::binary);

    if (!input.is_open()) {
        std::cerr << "[storage] open metadata file failed: "
                  << meta_path << std::endl;
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    *metadata = buffer.str();

    return true;
}

Packet StorageService::handleGetMetadata(const Packet& request) {
    std::string file_id;

    /*
     * GET_METADATA 的 body 和 DOWNLOAD_FILE 一样：
     * file_id=...
     *
     * 所以复用 parseDownloadBody。
     */
    if (!parseDownloadBody(request.body, &file_id)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "bad metadata body"
        );
    }

    std::string meta_path = buildMetaPath(file_id);

    std::string metadata;

    if (!readMetadata(meta_path, &metadata)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "read metadata failed"
        );
    }

    std::cout << "[storage] get metadata success"
              << ", file_id=" << file_id
              << ", meta_path=" << meta_path
              << std::endl;

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        metadata
    );
}

Packet StorageService::handleFetchBinlog(const Packet& request) {
    (void)request;

    std::string content;

    if (!binlog_.readAll(&content)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "read binlog failed"
        );
    }

    std::cout << "[storage] fetch binlog success"
              << ", size=" << content.size()
              << std::endl;

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        content
    );
}

bool StorageService::parseSyncPullBody(const std::string& body,
                                       std::string* src_ip,
                                       int* src_port) const {
    if (src_ip == nullptr || src_port == nullptr) {
        return false;
    }

    std::istringstream input(body);
    std::string line;

    while (std::getline(input, line)) {
        std::size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "src_ip") {
            *src_ip = value;
        } else if (key == "src_port") {
            try {
                *src_port = std::stoi(value);
            } catch (...) {
                return false;
            }
        }
    }

    return !src_ip->empty() && *src_port > 0;
}

bool StorageService::fetchRemoteBinlog(const std::string& src_ip,
                                       int src_port,
                                       std::string* binlog_content) const {
    if (binlog_content == nullptr) {
        return false;
    }

    TcpClient client(src_ip, src_port);

    Packet request = Protocol::makePacket(
        Command::FETCH_BINLOG,
        Status::OK,
        ""
    );

    Packet response;

    if (!client.sendPacket(request, &response)) {
        std::cerr << "[storage] fetch remote binlog failed"
                  << std::endl;
        return false;
    }

    if (response.header.status != Status::OK) {
        std::cerr << "[storage] source storage returned error: "
                  << response.body << std::endl;
        return false;
    }

    *binlog_content = response.body;
    return true;
}

bool StorageService::applyCreateFromSource(const std::string& src_ip,
                                           int src_port,
                                           const std::string& file_id,
                                           const std::string& filename,
                                           std::size_t size) {
    (void)size;

    /*
     * 向源 storage 下载文件。
     */
    TcpClient client(src_ip, src_port);

    std::string body = "file_id=" + file_id;

    Packet request = Protocol::makePacket(
        Command::DOWNLOAD_FILE,
        Status::OK,
        body
    );

    Packet response;

    if (!client.sendPacket(request, &response)) {
        std::cerr << "[storage] sync download failed, file_id="
                  << file_id << std::endl;
        return false;
    }

    if (response.header.status != Status::OK) {
        std::cerr << "[storage] sync download error, file_id="
                  << file_id
                  << ", error=" << response.body << std::endl;
        return false;
    }

    /*
     * 写入当前 storage 本地。
     */
    std::string real_path = buildRealPath(file_id);

    if (!writeFile(real_path, response.body)) {
        std::cerr << "[storage] sync write file failed, file_id="
                  << file_id << std::endl;
        return false;
    }

    /*
     * 同步 metadata。
     *
     * 当前不从源 storage 拉 meta，而是在目标 storage 重建一份。
     */
    std::string meta_path = buildMetaPath(file_id);

    if (!writeMetadata(meta_path,
                       file_id,
                       filename,
                       real_path,
                       response.body.size())) {
        std::cerr << "[storage] sync write metadata failed, file_id="
                  << file_id << std::endl;
        return false;
    }

    std::cout << "[storage] sync CREATE success"
              << ", file_id=" << file_id
              << ", real_path=" << real_path
              << std::endl;

    return true;
}

bool StorageService::applyDeleteFromSource(const std::string& file_id) {
    std::string real_path = buildRealPath(file_id);
    std::string meta_path = buildMetaPath(file_id);

    /*
     * 当前简单处理：
     * 删除失败时只打印 warning。
     *
     * 因为目标 storage 上可能本来就没有这个文件。
     */
    if (!deleteRealFile(real_path)) {
        std::cerr << "[storage] warning: sync delete file failed or not exists: "
                  << real_path << std::endl;
    }

    if (!deleteRealFile(meta_path)) {
        std::cerr << "[storage] warning: sync delete metadata failed or not exists: "
                  << meta_path << std::endl;
    }

    std::cout << "[storage] sync DELETE handled"
              << ", file_id=" << file_id
              << std::endl;

    return true;
}

bool StorageService::applyBinlogFromSource(const std::string& src_ip,
                                           int src_port,
                                           const std::string& binlog_content) {
    std::istringstream input(binlog_content);
    std::string line;

    bool all_ok = true;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> fields = splitByTab(line);

        if (fields.size() < 3) {
            std::cerr << "[storage] bad binlog line: "
                      << line << std::endl;
            all_ok = false;
            continue;
        }

        const std::string& op = fields[1];

        if (op == "CREATE") {
            if (fields.size() < 5) {
                std::cerr << "[storage] bad CREATE binlog line: "
                          << line << std::endl;
                all_ok = false;
                continue;
            }

            std::string file_id = fields[2];
            std::string filename = fields[3];

            std::size_t size = 0;
            try {
                size = static_cast<std::size_t>(
                    std::stoull(fields[4])
                );
            } catch (...) {
                size = 0;
            }

            if (!applyCreateFromSource(src_ip,
                                       src_port,
                                       file_id,
                                       filename,
                                       size)) {
                all_ok = false;
            }
        } else if (op == "DELETE") {
            std::string file_id = fields[2];

            if (!applyDeleteFromSource(file_id)) {
                all_ok = false;
            }
        } else {
            std::cerr << "[storage] unknown binlog op: "
                      << op << std::endl;
            all_ok = false;
        }
    }

    return all_ok;
}

Packet StorageService::handleSyncPull(const Packet& request) {
    std::string src_ip;
    int src_port = 0;

    if (!parseSyncPullBody(request.body, &src_ip, &src_port)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "bad sync pull body"
        );
    }

    std::cout << "[storage] sync pull start"
              << ", src_ip=" << src_ip
              << ", src_port=" << src_port
              << std::endl;

    std::string binlog_content;

    if (!fetchRemoteBinlog(src_ip, src_port, &binlog_content)) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "fetch remote binlog failed"
        );
    }

    bool ok = applyBinlogFromSource(src_ip,
                                    src_port,
                                    binlog_content);

    if (!ok) {
        return Protocol::makePacket(
            Command::RESPONSE,
            Status::ERROR,
            "apply binlog failed"
        );
    }

    std::cout << "[storage] sync pull success"
              << ", src_ip=" << src_ip
              << ", src_port=" << src_port
              << std::endl;

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::OK,
        "sync pull success"
    );
}
