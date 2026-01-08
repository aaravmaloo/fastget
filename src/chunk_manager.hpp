#pragma once
#include <vector>
#include <cstdint>
#include <mutex>

namespace fastget {

struct Chunk {
    size_t id;
    size_t start;
    size_t end;
    bool downloaded = false;
    bool in_progress = false;
};

class ChunkManager {
public:
    ChunkManager(size_t total_size, size_t initial_chunk_size = 1024 * 1024);

    Chunk* GetNextChunk();
    void MarkSuccess(size_t chunk_id, double speed);
    void MarkFailed(size_t chunk_id);

    size_t GetTotalChunks() const { return chunks_.size(); }
    size_t GetDownloadedChunks() const { return downloaded_count_; }
    size_t GetChunkSize() const { return current_chunk_size_; }
    
    bool IsFinished() const { return downloaded_count_ == chunks_.size(); }

private:
    void AdaptChunkSize(bool success, double speed);

    size_t total_size_;
    size_t current_chunk_size_;
    std::vector<Chunk> chunks_;
    size_t downloaded_count_ = 0;
    std::mutex manager_mutex_;

    // Performance tracking
    size_t success_streak_ = 0;
    size_t fail_streak_ = 0;
    const size_t STREAK_THRESHOLD = 3;
};

} // namespace fastget
