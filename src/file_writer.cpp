#include "file_writer.hpp"
#include <filesystem>

namespace fastget {

FileWriter::FileWriter(const std::string& filename) : filename_(filename) {}

FileWriter::~FileWriter() {
    Close();
}

bool FileWriter::Open() {
    file_.open(filename_, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        file_.clear();
        file_.open(filename_, std::ios::out | std::ios::binary);
        file_.close();
        file_.open(filename_, std::ios::in | std::ios::out | std::ios::binary);
    }
    return file_.is_open();
}

void FileWriter::PreAllocate(size_t size) {
    if (GetSize() >= size) return;
    
    std::lock_guard<std::mutex> lock(write_mutex_);
    file_.seekp(size - 1);
    file_.write("", 1);
    file_.flush();
}

bool FileWriter::WriteAt(size_t offset, const std::vector<char>& data) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    if (!file_.is_open()) return false;

    file_.seekp(offset);
    file_.write(data.data(), data.size());
    file_.flush();
    return true;
}

void FileWriter::Close() {
    if (file_.is_open()) {
        file_.close();
    }
}

bool FileWriter::Exists() const {
    return std::filesystem::exists(filename_);
}

size_t FileWriter::GetSize() const {
    if (!Exists()) return 0;
    return std::filesystem::file_size(filename_);
}

} // namespace fastget
