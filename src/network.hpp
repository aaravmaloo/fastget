#pragma once
#include <string>
#include <vector>
#include <curl/curl.h>

namespace fastget {

struct NetworkResponse {
    long status_code;
    std::string data;
    std::string error;
    bool success;
};

class NetworkLayer {
public:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    static long GetFileSize(const std::string& url);
    static bool DownloadChunk(const std::string& url, size_t start, size_t end, std::vector<char>& buffer);

    static double MeasureSpeed(const std::string& url, size_t size_bytes);
};

} // namespace fastget
