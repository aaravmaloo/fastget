#include "ui.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace fastget {

void UI::PrintHeader(const std::string& filename, size_t size, int connections) {
    std::cout << "Downloading: " << filename << std::endl;
    std::cout << "Size: " << FormatSize(size) << std::endl;
    std::cout << "Connections: " << connections << std::endl;
}

void UI::UpdateProgress(size_t downloaded, size_t total, double speed_bps, std::chrono::steady_clock::time_point start_time) {
    if (total == 0) return;
    double percent = (static_cast<double>(downloaded) / total) * 100.0;
    int barWidth = 30;
    int pos = static_cast<int>(barWidth * percent / 100.0);

    std::cout << "\x1b[2K\rProgress: " << std::fixed << std::setprecision(1) << std::setw(5) << percent << "% [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "█";
        else if (i == pos) std::cout << "█";
        else std::cout << "░";
    }
    std::cout << "] " << FormatSpeed(speed_bps);

    if (speed_bps > 0) {
        long remaining_bytes = total - downloaded;
        long eta_seconds = static_cast<long>(remaining_bytes / speed_bps);
        std::cout << " ETA: " << FormatDuration(eta_seconds);
    }

    std::cout << std::flush;
}

void UI::PrintFooter(bool success, const std::string& message) {
    std::cout << std::endl;
    if (success) {
        std::cout << "Download complete!" << std::endl;
    } else {
        std::cerr << "Download failed: " << message << std::endl;
    }
}

void UI::PrintSummary(size_t total, size_t downloaded, double avg_speed_bps, long duration_seconds, bool resumed, size_t resumed_bytes, int connections) {
    std::cout << "Summary" << std::endl;
    std::cout << "Total: " << FormatSize(total) << std::endl;
    std::cout << "Downloaded: " << FormatSize(downloaded) << std::endl;
    std::cout << "Average speed: " << FormatSpeed(avg_speed_bps) << std::endl;
    std::cout << "Time: " << FormatDuration(duration_seconds) << std::endl;
    if (resumed) {
        std::cout << "Resumed: " << FormatSize(resumed_bytes) << std::endl;
    }
    std::cout << "Connections: " << connections << std::endl;
}

std::string UI::FormatSize(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double dbytes = static_cast<double>(bytes);
    while (dbytes >= 1024 && i < 4) {
        dbytes /= 1024;
        i++;
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << dbytes << " " << units[i];
    return ss.str();
}

std::string UI::FormatSpeed(double speed_bps) {
    return FormatSize(static_cast<size_t>(speed_bps)) + "/s";
}

std::string UI::FormatDuration(long seconds) {
    if (seconds < 60) return std::to_string(seconds) + "s";
    if (seconds < 3600) return std::to_string(seconds / 60) + "m " + std::to_string(seconds % 60) + "s";
    return std::to_string(seconds / 3600) + "h " + std::to_string((seconds % 3600) / 60) + "m";
}

}
