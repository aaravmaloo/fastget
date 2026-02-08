#pragma once
#include "network.hpp"
#include "file_writer.hpp"
#include "chunk_manager.hpp"
#include "ui.hpp"
#include "resume_state.hpp"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>

namespace fastget {

struct DownloadOptions {
    int num_threads = 8;
    size_t max_rate = 0;
    int retries = 2;
    int retry_delay_ms = 500;
    long timeout_ms = 0;
    long connect_timeout_ms = 0;
    bool verify_tls = false;
    bool resume = true;
    std::vector<std::string> headers;
    std::string user_agent;
};

class Downloader {
public:
    Downloader(const std::string& url, const std::vector<std::string>& mirrors, const std::string& output_path, const DownloadOptions& options);
    
    bool Start();
    void Pause();
    void Resume();

    size_t GetTotalSize() const { return total_size_; }
    size_t GetDownloadedSize() const { return downloaded_size_; }

private:
    void DownloadThread();
    void ProgressWatcher();
    std::string ResumePath() const;
    NetworkOptions BuildNetworkOptions() const;
    void InitializeResumeState();
    void ApplyResumeState();

    std::string url_;
    std::vector<std::string> mirrors_;
    std::string output_path_;
    DownloadOptions options_;
    
    size_t total_size_ = 0;
    std::atomic<size_t> downloaded_size_{0};
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};

    FileWriter writer_;
    std::unique_ptr<ChunkManager> chunk_manager_;
    std::vector<std::thread> threads_;
    std::chrono::steady_clock::time_point start_time_;
    ResumeState resume_state_;
    std::atomic<size_t> resumed_bytes_{0};
};

}
