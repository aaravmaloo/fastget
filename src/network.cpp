
#include "network.hpp"
#include <iostream>

namespace fastget {

size_t NetworkLayer::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::vector<char>* buffer = static_cast<std::vector<char>*>(userp);
    buffer->insert(buffer->end(), static_cast<char*>(contents), static_cast<char*>(contents) + totalSize);
    return totalSize;
}

static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    std::string header(buffer, size * nitems);
    size_t pos = header.find("Content-Range:");
    if (pos == std::string::npos) pos = header.find("content-range:");
    
    if (pos != std::string::npos) {
        size_t slash_pos = header.find('/', pos);
        if (slash_pos != std::string::npos) {
            try {
                *static_cast<long*>(userdata) = std::stol(header.substr(slash_pos + 1));
            } catch (...) {}
        }
    }
    return size * nitems;
}

long NetworkLayer::GetFileSize(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    long fileSize = -1;
    auto setup = [&](bool is_head) {
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "fastget/1.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        if (is_head) {
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        } else {
            curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &fileSize);
        }
    };

    setup(true);
    if (curl_easy_perform(curl) == CURLE_OK) {
        double cl;
        if (curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl) == CURLE_OK && cl > 0) {
            fileSize = static_cast<long>(cl);
        }
    }

    if (fileSize <= 0) {
        setup(false);
        curl_easy_perform(curl);
    }

    curl_easy_cleanup(curl);
    return fileSize;
}

bool NetworkLayer::DownloadChunk(const std::string& url, size_t start, size_t end, std::vector<char>& buffer) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string range = std::to_string(start) + "-" + std::to_string(end);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "fastget/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);

    return (res == CURLE_OK && (response_code == 200 || response_code == 206));
}

} // namespace fastget
