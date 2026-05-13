#include "storage/Binlog.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

namespace {

void ensureParentDir(const std::string& filename)
{
    std::size_t pos = filename.find_last_of('/');
    if (pos == std::string::npos) {
        return;
    }

    std::string dir = filename.substr(0, pos);
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

} // namespace

Binlog::Binlog(const std::string& binlog_path)
    : binlog_path_(binlog_path) {
}

bool Binlog::appendCreate(const std::string& file_id,
                          const std::string& filename,
                          std::size_t size) {
    std::ostringstream oss;

    /*
     * 日志格式：
     * timestamp<TAB>CREATE<TAB>file_id<TAB>filename<TAB>size
     */
    oss << std::time(nullptr) << '\t'
        << "CREATE" << '\t'
        << file_id << '\t'
        << filename << '\t'
        << size;

    return appendLine(oss.str());
}

bool Binlog::appendDelete(const std::string& file_id) {
    std::ostringstream oss;

    /*
     * 日志格式：
     * timestamp<TAB>DELETE<TAB>file_id
     */
    oss << std::time(nullptr) << '\t'
        << "DELETE" << '\t'
        << file_id;

    return appendLine(oss.str());
}

bool Binlog::appendLine(const std::string& line) {
    ensureParentDir(binlog_path_);

    std::ofstream output(binlog_path_.c_str(), std::ios::app);

    if (!output.is_open()) {
        std::cerr << "[storage] open binlog failed: "
                  << binlog_path_ << std::endl;
        return false;
    }

    output << line << '\n';

    return output.good();
}

bool Binlog::readAll(std::string* content) const {
    if (content == nullptr) {
        return false;
    }

    std::ifstream input(binlog_path_.c_str(), std::ios::binary);

    if (!input.is_open()) {
        std::cerr << "[storage] open binlog for read failed: "
                  << binlog_path_ << std::endl;
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    *content = buffer.str();
    return true;
}
