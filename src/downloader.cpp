#include "downloader.hpp"
#include <iostream>
#include <chrono>
#include <filesystem>

namespace fastget {

Downloader::Downloader(const std::string& url, const std::vector<std::string>& mirrors, const std::string& output_path, const DownloadOptions& options)
    : url_(url), mirrors_(mirrors), output_path_(output_path), options_(options), writer_(output_path), resume_state_(ResumePath()) {
    std::vector<std::string> all_urls = mirrors_;
    all_urls.insert(all_urls.begin(), url_);

    long size = -1;
    NetworkOptions net_options = BuildNetworkOptions();
    for (const auto& u : all_urls) {
        size = NetworkLayer::GetFileSize(u, net_options);
        if (size > 0) break;
    }

    if (size > 0) {
        total_size_ = static_cast<size_t>(size);
    } else {
        total_size_ = 0;
    }
}

bool Downloader::Start() {
    if (total_size_ <= 0) {
        return false;
    }

    if (!writer_.Open()) {
        return false;
    }

    writer_.PreAllocate(total_size_);

    InitializeResumeState();
    if (!chunk_manager_) {
        return false;
    }

    ApplyResumeState();

    running_ = true;
    start_time_ = std::chrono::steady_clock::now();

    UI::PrintHeader(output_path_, total_size_, options_.num_threads);

    if (chunk_manager_->IsFinished()) {
        running_ = false;
        UI::PrintFooter(true);
        UI::PrintSummary(total_size_, downloaded_size_, 0.0, 0, resumed_bytes_ > 0, resumed_bytes_, options_.num_threads);
        if (options_.resume) {
            std::filesystem::remove(ResumePath());
        }
        return true;
    }

    for (int i = 0; i < options_.num_threads; ++i) {
        threads_.emplace_back(&Downloader::DownloadThread, this);
    }

    std::thread watcher(&Downloader::ProgressWatcher, this);

    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }

    running_ = false;
    if (watcher.joinable()) watcher.join();

    bool finished = chunk_manager_->IsFinished();
    if (options_.resume) {
        resume_state_.Save();
        if (finished) {
            std::filesystem::remove(ResumePath());
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end_time - start_time_;
    double avg_speed = diff.count() > 0 ? static_cast<double>(downloaded_size_) / diff.count() : 0.0;

    UI::PrintFooter(finished, finished ? "" : "Could not complete download.");
    UI::PrintSummary(total_size_, downloaded_size_, avg_speed, static_cast<long>(diff.count()), resumed_bytes_ > 0, resumed_bytes_, options_.num_threads);

    return finished;
}

void Downloader::DownloadThread() {
    NetworkOptions net_options = BuildNetworkOptions();
    while (running_ && !chunk_manager_->IsFinished()) {
        if (paused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        Chunk* chunk = chunk_manager_->GetNextChunk();
        if (!chunk) break;

        std::vector<char> buffer;
        buffer.reserve(chunk->end - chunk->start + 1);

        std::vector<std::string> all_urls = mirrors_;
        all_urls.insert(all_urls.begin(), url_);

        bool success = false;
        double speed = 0.0;

        for (int attempt = 0; attempt <= options_.retries && running_; ++attempt) {
            for (const auto& current_url : all_urls) {
                buffer.clear();
                auto start_time = std::chrono::steady_clock::now();
                std::string error;
                if (NetworkLayer::DownloadChunk(current_url, chunk->start, chunk->end, buffer, net_options, &error)) {
                    auto end_time = std::chrono::steady_clock::now();
                    std::chrono::duration<double> diff = end_time - start_time;
                    if (diff.count() > 0) {
                        speed = buffer.size() / diff.count();
                    }
                    success = true;
                    break;
                }
            }
            if (success) break;
            if (options_.retry_delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(options_.retry_delay_ms));
            }
        }

        if (success) {
            writer_.WriteAt(chunk->start, buffer);
            downloaded_size_ += buffer.size();
            chunk_manager_->MarkSuccess(chunk->id, speed);
            if (options_.resume) {
                resume_state_.MarkCompleted(chunk->id);
                resume_state_.MaybeSave();
            }
        } else {
            chunk_manager_->MarkFailed(chunk->id);
        }
    }
}

void Downloader::ProgressWatcher() {
    while (running_) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff = now - start_time_;
        double total_speed = diff.count() > 0 ? downloaded_size_ / diff.count() : 0.0;

        UI::UpdateProgress(downloaded_size_, total_size_, total_speed, start_time_);

        if (chunk_manager_->IsFinished()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void Downloader::Pause() {
    paused_ = true;
    if (options_.resume) {
        resume_state_.Save();
    }
}

void Downloader::Resume() {
    paused_ = false;
}

std::string Downloader::ResumePath() const {
    return output_path_ + ".fastget";
}

NetworkOptions Downloader::BuildNetworkOptions() const {
    NetworkOptions options;
    options.timeout_ms = options_.timeout_ms;
    options.connect_timeout_ms = options_.connect_timeout_ms;
    options.verify_tls = options_.verify_tls;
    options.user_agent = options_.user_agent;
    options.headers = options_.headers;
    if (options_.max_rate > 0) {
        int threads = options_.num_threads > 0 ? options_.num_threads : 1;
        options.max_speed = options_.max_rate / static_cast<size_t>(threads);
        if (options.max_speed == 0) options.max_speed = options_.max_rate;
    }
    return options;
}

void Downloader::InitializeResumeState() {
    size_t chunk_size = 1024 * 1024;
    size_t saved_chunk_size = 0;
    size_t saved_chunk_count = 0;

    if (options_.resume && resume_state_.Load(total_size_, &saved_chunk_size, &saved_chunk_count)) {
        if (saved_chunk_size > 0) chunk_size = saved_chunk_size;
        chunk_manager_ = std::make_unique<ChunkManager>(total_size_, chunk_size);
        return;
    }

    chunk_manager_ = std::make_unique<ChunkManager>(total_size_, chunk_size);
    if (options_.resume && chunk_manager_) {
        resume_state_.Initialize(total_size_, chunk_manager_->GetChunkSize(), chunk_manager_->GetTotalChunks());
    }
}

void Downloader::ApplyResumeState() {
    if (!options_.resume || !chunk_manager_) return;
    auto completed = resume_state_.GetCompletedChunks();
    size_t resumed_bytes = 0;
    for (size_t chunk_id : completed) {
        size_t start = 0;
        size_t end = 0;
        if (chunk_manager_->GetChunkRange(chunk_id, start, end)) {
            resumed_bytes += (end - start + 1);
            chunk_manager_->MarkCompleted(chunk_id);
        }
    }
    resumed_bytes_ = resumed_bytes;
    downloaded_size_ = resumed_bytes;
}

}
