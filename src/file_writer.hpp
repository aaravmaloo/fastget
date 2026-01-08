#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <vector>

namespace fastget {

class FileWriter {
public:
    FileWriter(const std::string& filename);
    ~FileWriter();

    bool Open();
    void PreAllocate(size_t size);
    bool WriteAt(size_t offset, const std::vector<char>& data);
    void Close();

    bool Exists() const;
    size_t GetSize() const;

private:
    std::string filename_;
    std::fstream file_;
    std::mutex write_mutex_;
};

} // namespace fastget
