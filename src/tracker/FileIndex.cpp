#include "tracker/FileIndex.h"
#include <fstream>
#include <iostream>
#include <sstream>

/*
 * 按 tab 分割一行。
 *
 * file_index.dat 每行格式：
 * file_id<TAB>group_name<TAB>ip<TAB>port
 */
static bool splitIndexLine(const std::string& line,
                           std::string* file_id,
                           StorageNode* node) {
    if (file_id == nullptr || node == nullptr) {
        return false;
    }

    std::istringstream iss(line);

    std::string port_text;

    if (!std::getline(iss, *file_id, '\t')) {
        return false;
    }

    if (!std::getline(iss, node->group_name, '\t')) {
        return false;
    }

    if (!std::getline(iss, node->ip, '\t')) {
        return false;
    }

    if (!std::getline(iss, port_text, '\t')) {
        return false;
    }

    try {
        node->port = std::stoi(port_text);
    } catch (...) {
        return false;
    }

    node->base_path = "";
    node->store_path0 = "";
    node->last_heartbeat = 0;
    node->online = true;

    return !file_id->empty() &&
           !node->group_name.empty() &&
           !node->ip.empty() &&
           node->port > 0;
}

void FileIndex::put(const std::string& file_id,
                    const StorageNode& node) {
    index_[file_id] = node;
}

bool FileIndex::get(const std::string& file_id,
                    StorageNode* node) const {
    if (node == nullptr) {
        return false;
    }

    std::unordered_map<std::string, StorageNode>::const_iterator it =
        index_.find(file_id);

    if (it == index_.end()) {
        return false;
    }

    *node = it->second;
    return true;
}

bool FileIndex::remove(const std::string& file_id) {
    return index_.erase(file_id) > 0;
}

std::size_t FileIndex::size() const {
    return index_.size();
}

bool FileIndex::loadFromFile(const std::string& filename) {
    std::ifstream input(filename.c_str());

    /*
     * 文件不存在时不算错误。
     * 第一次启动 tracker，本来就没有 file_index.dat。
     */
    if (!input.is_open()) {
        std::cout << "[tracker] file index not found, start with empty index: "
                  << filename << std::endl;
        return true;
    }

    index_.clear();

    std::string line;
    int line_no = 0;

    while (std::getline(input, line)) {
        ++line_no;

        if (line.empty()) {
            continue;
        }

        std::string file_id;
        StorageNode node;

        if (!splitIndexLine(line, &file_id, &node)) {
            std::cerr << "[tracker] bad file index line "
                      << line_no << ": " << line << std::endl;
            return false;
        }

        index_[file_id] = node;
    }

    std::cout << "[tracker] file index loaded"
              << ", file=" << filename
              << ", count=" << index_.size()
              << std::endl;

    return true;
}

bool FileIndex::saveToFile(const std::string& filename) const {
    std::ofstream output(filename.c_str(), std::ios::trunc);

    if (!output.is_open()) {
        std::cerr << "[tracker] open file index failed: "
                  << filename << std::endl;
        return false;
    }

    /*
     * 每行格式：
     * file_id<TAB>group_name<TAB>ip<TAB>port
     */
    for (std::unordered_map<std::string, StorageNode>::const_iterator it =
             index_.begin();
         it != index_.end();
         ++it) {
        const std::string& file_id = it->first;
        const StorageNode& node = it->second;

        output << file_id << '\t'
               << node.group_name << '\t'
               << node.ip << '\t'
               << node.port << '\n';
    }

    return output.good();
}

void FileIndex::dump() const {
    std::cout << "[tracker] file index dump, total="
              << index_.size() << std::endl;

    for (std::unordered_map<std::string, StorageNode>::const_iterator it =
             index_.begin();
         it != index_.end();
         ++it) {
        const std::string& file_id = it->first;
        const StorageNode& node = it->second;

        std::cout << "  - file_id=" << file_id
                  << ", group=" << node.group_name
                  << ", ip=" << node.ip
                  << ", port=" << node.port
                  << std::endl;
    }
}