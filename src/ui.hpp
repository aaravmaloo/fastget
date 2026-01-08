#pragma once
#include <string>
#include <chrono>

namespace fastget {

class UI {
public:
    static void PrintHeader(const std::string& filename, size_t size, int connections);
    static void UpdateProgress(size_t downloaded, size_t total, double speed_bps, std::chrono::steady_clock::time_point start_time);
    static void PrintFooter(bool success, const std::string& message = "");

private:
    static std::string FormatSize(size_t bytes);
    static std::string FormatSpeed(double speed_bps);
    static std::string FormatDuration(long seconds);
};

} // namespace fastget
