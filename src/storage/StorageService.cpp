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

StorageService::StorageService(const std::string& group_name,
                               const std::string& store_path0)
    : group_name_(group_name),
      store_path0_(store_path0) {
}

Packet StorageService::handlePacket(const Packet& request
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

    return Protocol::makePacket(
        Command::RESPONSE,
        Status::ERROR,
        "unknown storage command"
    );
}

bool StorageService:parseUploadBody(const std::string& body,
                                    std::string& filename,
                                    std::string& content) const
{
    if(filename == nullptr || content == nullptr) {
        return false;
    }

    cosnt std::string filename_key = "filename=";
    cosnt std::string content_key = "\ncontent=";

   id(body.find(filename_key) != 0) {
        return false;
    }

    std::size_t content_pos = body.find(content_key);
    if(content_pos == std::string::npos) {
        return false;
    }

    *filename = body.substr(filename_key.size(), content_pos - filename_key.size());
    *content = body.substr(content_pos + content_key.size());

    return !filename->empty();
}

std::string StorageService::getStorePath(const std::string& filename) const
{
    std::ostringstream oss;

    oss << group_name;
        << "/M00/00/00/"
        << std::time(nullptr)
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
    mkdir((store_path0_ + "/M00").c_str(), 0755);
    mkdir((store_path0_ + "/M00/00").c_str(), 0755);
    mkdir((store_path0_ + "/M00/00/00").c_str(), 0755);

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
        return Protcool::makePacket(
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

    return !file_id->empty();
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

    /*
    * metadata 删除失败不一定要让整个 delete 失败。
    * 因为真实文件已经删除了。
    * 这里先打印警告。
    */
    if (!deleteRealFile(meta_path)) {
        std::cerr << "[storage] warning: delete metadata failed: "
                << meta_path << std::endl;
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