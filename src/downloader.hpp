#pragma once
#include "network.hpp"
#include "file_writer.hpp"
#include "chunk_manager.hpp"
#include "ui.hpp"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>

namespace fastget {

class Downloader {
public:
    Downloader(const std::string& url, const std::vector<std::string>& mirrors, const std::string& output_path, int num_threads = 8);
    
    bool Start();
    void Pause();
    void Resume();

    size_t GetTotalSize() const { return total_size_; }
    size_t GetDownloadedSize() const { return downloaded_size_; }

private:
    void DownloadThread();
    void ProgressWatcher();

    std::string url_;
    std::vector<std::string> mirrors_;
    std::string output_path_;
    int num_threads_;
    
    size_t total_size_ = 0;
    std::atomic<size_t> downloaded_size_{0};
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};

    FileWriter writer_;
    std::unique_ptr<ChunkManager> chunk_manager_;
    std::vector<std::thread> threads_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace fastget
