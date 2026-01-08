#include "downloader.hpp"
#include <iostream>
#include <chrono>

namespace fastget {

Downloader::Downloader(const std::string& url, const std::string& output_path, int num_threads)
    : url_(url), output_path_(output_path), num_threads_(num_threads), 
      writer_(output_path) {
    
    long size = NetworkLayer::GetFileSize(url_);
    if (size > 0) {
        total_size_ = static_cast<size_t>(size);
        chunk_manager_ = std::make_unique<ChunkManager>(total_size_);
    } else {
        total_size_ = 0;
    }
}

bool Downloader::Start() {
    if (total_size_ <= 0 || !chunk_manager_) {
        return false;
    }

    if (!writer_.Open()) {
        return false;
    }

    writer_.PreAllocate(total_size_);
    
    running_ = true;
    start_time_ = std::chrono::steady_clock::now();

    UI::PrintHeader(output_path_, total_size_, num_threads_);

    for (int i = 0; i < num_threads_; ++i) {
        threads_.emplace_back(&Downloader::DownloadThread, this);
    }

    std::thread watcher(&Downloader::ProgressWatcher, this);

    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }

    running_ = false;
    if (watcher.joinable()) watcher.join();

    return chunk_manager_->IsFinished();
}

void Downloader::DownloadThread() {
    while (running_ && !chunk_manager_->IsFinished()) {
        if (paused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        Chunk* chunk = chunk_manager_->GetNextChunk();
        if (!chunk) break;

        std::vector<char> buffer;
        buffer.reserve(chunk->end - chunk->start + 1);

        auto startTime = std::chrono::steady_clock::now();
        if (NetworkLayer::DownloadChunk(url_, chunk->start, chunk->end, buffer)) {
            writer_.WriteAt(chunk->start, buffer);
            
            auto endTime = std::chrono::steady_clock::now();
            std::chrono::duration<double> diff = endTime - startTime;
            double speed = buffer.size() / diff.count();

            downloaded_size_ += buffer.size();
            chunk_manager_->MarkSuccess(chunk->id, speed);
        } else {
            chunk_manager_->MarkFailed(chunk->id);
        }
    }
}

void Downloader::ProgressWatcher() {
    while (running_) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff = now - start_time_;
        double total_speed = downloaded_size_ / diff.count();

        UI::UpdateProgress(downloaded_size_, total_size_, total_speed, start_time_);
        
        if (chunk_manager_->IsFinished()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void Downloader::Pause() {
    paused_ = true;
}

void Downloader::Resume() {
    paused_ = false;
}

} // namespace fastget
