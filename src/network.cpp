#include "network.hpp"

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

static void ApplyNetworkOptions(CURL* curl, const NetworkOptions& options, curl_slist** headers) {
    if (!options.user_agent.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, options.user_agent.c_str());
    } else {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "fastget/1.1");
    }
    if (options.timeout_ms > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, options.timeout_ms);
    }
    if (options.connect_timeout_ms > 0) {
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, options.connect_timeout_ms);
    }
    if (options.max_speed > 0) {
        curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, static_cast<curl_off_t>(options.max_speed));
    }
    if (options.verify_tls) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    } else {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    if (!options.headers.empty()) {
        for (const auto& header : options.headers) {
            *headers = curl_slist_append(*headers, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headers);
    }
}

long NetworkLayer::GetFileSize(const std::string& url, const NetworkOptions& options) {
    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    long fileSize = -1;
    auto setup = [&](bool is_head) {
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        if (is_head) {
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        } else {
            curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &fileSize);
        }
    };

    curl_slist* headers = nullptr;
    setup(true);
    ApplyNetworkOptions(curl, options, &headers);
    if (curl_easy_perform(curl) == CURLE_OK) {
        double cl;
        if (curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl) == CURLE_OK && cl > 0) {
            fileSize = static_cast<long>(cl);
        }
    }

    if (fileSize <= 0) {
        if (headers) {
            curl_slist_free_all(headers);
            headers = nullptr;
        }
        setup(false);
        ApplyNetworkOptions(curl, options, &headers);
        curl_easy_perform(curl);
    }

    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    return fileSize;
}

bool NetworkLayer::DownloadChunk(const std::string& url, size_t start, size_t end, std::vector<char>& buffer, const NetworkOptions& options, std::string* error) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string range = std::to_string(start) + "-" + std::to_string(end);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_slist* headers = nullptr;
    ApplyNetworkOptions(curl, options, &headers);
    char error_buffer[CURL_ERROR_SIZE];
    error_buffer[0] = '\0';
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (error) {
        if (res != CURLE_OK && error_buffer[0] != '\0') {
            *error = error_buffer;
        } else if (res != CURLE_OK) {
            *error = curl_easy_strerror(res);
        } else {
            error->clear();
        }
    }

    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && (response_code == 200 || response_code == 206));
}

}
