#include "storage/Binlog.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

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
    std::ofstream output(binlog_path_.c_str(), std::ios::app);

    if (!output.is_open()) {
        std::cerr << "[storage] open binlog failed: "
                  << binlog_path_ << std::endl;
        return false;
    }

    output << line << '\n';

    return output.good();
}

