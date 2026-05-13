#include "tracker/FileIndex.h"

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