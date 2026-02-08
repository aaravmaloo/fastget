#pragma once
#include <string>
#include <vector>
#include <curl/curl.h>

namespace fastget {

struct NetworkOptions {
    long timeout_ms = 0;
    long connect_timeout_ms = 0;
    size_t max_speed = 0;
    bool verify_tls = false;
    std::string user_agent;
    std::vector<std::string> headers;
};

class NetworkLayer {
public:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    static long GetFileSize(const std::string& url, const NetworkOptions& options);
    static bool DownloadChunk(const std::string& url, size_t start, size_t end, std::vector<char>& buffer, const NetworkOptions& options, std::string* error);
};

}
