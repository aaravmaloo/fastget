#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <mutex>

namespace fastget {

class ResumeState {
public:
    explicit ResumeState(const std::string& path);

    bool Load(size_t expected_total_size, size_t* out_chunk_size, size_t* out_chunk_count);
    void Initialize(size_t total_size, size_t chunk_size, size_t chunk_count);
    bool IsInitialized() const;
    bool IsChunkComplete(size_t chunk_id) const;
    void MarkCompleted(size_t chunk_id);
    std::vector<size_t> GetCompletedChunks() const;
    void Save();
    void MaybeSave();

private:
    void SaveLocked();

    std::string path_;
    size_t total_size_ = 0;
    size_t chunk_size_ = 0;
    size_t chunk_count_ = 0;
    std::vector<uint8_t> completed_;
    bool initialized_ = false;
    bool dirty_ = false;
    std::chrono::steady_clock::time_point last_save_;
    mutable std::mutex mutex_;
};

}
