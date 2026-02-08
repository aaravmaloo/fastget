#pragma once
#include <string>
#include <chrono>

namespace fastget {

class UI {
public:
    static void PrintHeader(const std::string& filename, size_t size, int connections);
    static void UpdateProgress(size_t downloaded, size_t total, double speed_bps, std::chrono::steady_clock::time_point start_time);
    static void PrintFooter(bool success, const std::string& message = "");
    static void PrintSummary(size_t total, size_t downloaded, double avg_speed_bps, long duration_seconds, bool resumed, size_t resumed_bytes, int connections);

private:
    static std::string FormatSize(size_t bytes);
    static std::string FormatSpeed(double speed_bps);
    static std::string FormatDuration(long seconds);
};

}
