#include "chunk_manager.hpp"
#include <algorithm>

namespace fastget {

ChunkManager::ChunkManager(size_t total_size, size_t initial_chunk_size)
    : total_size_(total_size), current_chunk_size_(initial_chunk_size) {
    
    if (total_size_ == 0 || initial_chunk_size == 0) return;

    size_t offset = 0;
    size_t id = 0;
    const size_t MAX_CHUNKS = 1000000;

    while (offset < total_size_ && id < MAX_CHUNKS) {
        size_t end = std::min(offset + current_chunk_size_ - 1, total_size_ - 1);
        chunks_.push_back({id++, offset, end});
        offset = end + 1;
    }
}

Chunk* ChunkManager::GetNextChunk() {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    for (auto& chunk : chunks_) {
        if (!chunk.downloaded && !chunk.in_progress) {
            chunk.in_progress = true;
            return &chunk;
        }
    }
    return nullptr;
}

void ChunkManager::MarkSuccess(size_t chunk_id, double speed) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    if (chunk_id < chunks_.size()) {
        chunks_[chunk_id].downloaded = true;
        chunks_[chunk_id].in_progress = false;
        downloaded_count_++;
        AdaptChunkSize(true, speed);
    }
}

void ChunkManager::MarkFailed(size_t chunk_id) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    if (chunk_id < chunks_.size()) {
        chunks_[chunk_id].in_progress = false;
        AdaptChunkSize(false, 0);
    }
}

bool ChunkManager::MarkCompleted(size_t chunk_id) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    if (chunk_id >= chunks_.size()) return false;
    if (!chunks_[chunk_id].downloaded) {
        chunks_[chunk_id].downloaded = true;
        chunks_[chunk_id].in_progress = false;
        downloaded_count_++;
    }
    return true;
}

bool ChunkManager::GetChunkRange(size_t chunk_id, size_t& start, size_t& end) const {
    if (chunk_id >= chunks_.size()) return false;
    start = chunks_[chunk_id].start;
    end = chunks_[chunk_id].end;
    return true;
}

void ChunkManager::AdaptChunkSize(bool success, double speed) {
    if (success) {
        fail_streak_ = 0;
        success_streak_++;
        if (success_streak_ >= STREAK_THRESHOLD) {
            current_chunk_size_ *= 2;
            success_streak_ = 0;
            if (current_chunk_size_ > 16 * 1024 * 1024) {
                current_chunk_size_ = 16 * 1024 * 1024;
            }
        }
    } else {
        success_streak_ = 0;
        fail_streak_++;
        if (fail_streak_ >= 1) {
            current_chunk_size_ /= 2;
            if (current_chunk_size_ < 512 * 1024) {
                current_chunk_size_ = 512 * 1024;
            }
            fail_streak_ = 0;
        }
    }
}

}
