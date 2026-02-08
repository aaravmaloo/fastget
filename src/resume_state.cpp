#include "resume_state.hpp"
#include <fstream>
#include <filesystem>

namespace fastget {

static constexpr char kMagic[] = "FASTGET1";
static constexpr size_t kMagicSize = 8;

ResumeState::ResumeState(const std::string& path) : path_(path) {}

bool ResumeState::Load(size_t expected_total_size, size_t* out_chunk_size, size_t* out_chunk_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!std::filesystem::exists(path_)) return false;
    std::ifstream file(path_, std::ios::binary);
    if (!file.is_open()) return false;

    char magic[kMagicSize];
    file.read(magic, sizeof(magic));
    if (!file || std::string(magic, sizeof(magic)) != std::string(kMagic, sizeof(magic))) return false;

    uint64_t total_size = 0;
    uint64_t chunk_size = 0;
    uint64_t chunk_count = 0;
    file.read(reinterpret_cast<char*>(&total_size), sizeof(total_size));
    file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size));
    file.read(reinterpret_cast<char*>(&chunk_count), sizeof(chunk_count));
    if (!file) return false;
    if (total_size != expected_total_size) return false;

    std::vector<uint8_t> completed(chunk_count, 0);
    file.read(reinterpret_cast<char*>(completed.data()), completed.size());
    if (!file) return false;

    total_size_ = static_cast<size_t>(total_size);
    chunk_size_ = static_cast<size_t>(chunk_size);
    chunk_count_ = static_cast<size_t>(chunk_count);
    completed_ = std::move(completed);
    initialized_ = true;
    dirty_ = false;
    last_save_ = std::chrono::steady_clock::now();

    if (out_chunk_size) *out_chunk_size = chunk_size_;
    if (out_chunk_count) *out_chunk_count = chunk_count_;
    return true;
}

void ResumeState::Initialize(size_t total_size, size_t chunk_size, size_t chunk_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    total_size_ = total_size;
    chunk_size_ = chunk_size;
    chunk_count_ = chunk_count;
    completed_.assign(chunk_count_, 0);
    initialized_ = true;
    dirty_ = true;
    last_save_ = std::chrono::steady_clock::now();
}

bool ResumeState::IsInitialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_;
}

bool ResumeState::IsChunkComplete(size_t chunk_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_ || chunk_id >= completed_.size()) return false;
    return completed_[chunk_id] != 0;
}

void ResumeState::MarkCompleted(size_t chunk_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_ || chunk_id >= completed_.size()) return;
    if (completed_[chunk_id] == 0) {
        completed_[chunk_id] = 1;
        dirty_ = true;
    }
}

std::vector<size_t> ResumeState::GetCompletedChunks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<size_t> result;
    if (!initialized_) return result;
    result.reserve(completed_.size());
    for (size_t i = 0; i < completed_.size(); ++i) {
        if (completed_[i] != 0) result.push_back(i);
    }
    return result;
}

void ResumeState::Save() {
    std::lock_guard<std::mutex> lock(mutex_);
    SaveLocked();
}

void ResumeState::MaybeSave() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!dirty_) return;
    auto now = std::chrono::steady_clock::now();
    if (now - last_save_ < std::chrono::seconds(1)) return;
    SaveLocked();
}

void ResumeState::SaveLocked() {
    if (!initialized_) return;
    std::filesystem::path temp_path = path_ + ".tmp";
    std::ofstream file(temp_path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) return;

    file.write(kMagic, kMagicSize);
    uint64_t total_size = total_size_;
    uint64_t chunk_size = chunk_size_;
    uint64_t chunk_count = chunk_count_;
    file.write(reinterpret_cast<const char*>(&total_size), sizeof(total_size));
    file.write(reinterpret_cast<const char*>(&chunk_size), sizeof(chunk_size));
    file.write(reinterpret_cast<const char*>(&chunk_count), sizeof(chunk_count));
    if (!completed_.empty()) {
        file.write(reinterpret_cast<const char*>(completed_.data()), completed_.size());
    }
    file.flush();
    file.close();
    std::filesystem::rename(temp_path, path_);
    dirty_ = false;
    last_save_ = std::chrono::steady_clock::now();
}

}
