#include "downloader.hpp"
#include "verifier.hpp"
#include "ui.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <csignal>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

using namespace fastget;

Downloader* global_downloader = nullptr;

static std::string Trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) start++;
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) end--;
    return value.substr(start, end - start);
}

static std::string BaseNameFromUrl(const std::string& url) {
    std::string cleaned = url;
    size_t query = cleaned.find_first_of("?#");
    if (query != std::string::npos) cleaned = cleaned.substr(0, query);
    size_t last_slash = cleaned.find_last_of('/');
    if (last_slash != std::string::npos && last_slash + 1 < cleaned.size()) {
        return cleaned.substr(last_slash + 1);
    }
    return "downloaded_file";
}

static size_t ParseSize(const std::string& value) {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) return 0;
    size_t i = 0;
    while (i < trimmed.size() && (std::isdigit(static_cast<unsigned char>(trimmed[i])) || trimmed[i] == '.')) i++;
    double number = std::stod(trimmed.substr(0, i));
    std::string suffix = Trim(trimmed.substr(i));
    std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    size_t multiplier = 1;
    if (suffix == "k" || suffix == "kb") multiplier = 1024;
    else if (suffix == "m" || suffix == "mb") multiplier = 1024 * 1024;
    else if (suffix == "g" || suffix == "gb") multiplier = 1024ULL * 1024ULL * 1024ULL;

    return static_cast<size_t>(number * multiplier);
}

static void PrintUsage() {
    std::cout << "Usage: fastget <url> [options]\n"
              << "Options:\n"
              << "  --output <path>         Specify output file path\n"
              << "  --output-dir <path>     Directory for output files\n"
              << "  --threads <n>           Number of download threads\n"
              << "  --mirrors <urls>        Comma-separated list of mirror URLs\n"
              << "  --sha256 <hash>         Verify SHA-256 checksum\n"
              << "  --md5 <hash>            Verify MD5 checksum\n"
              << "  --sha1 <hash>           Verify SHA-1 checksum\n"
              << "  --sha512 <hash>         Verify SHA-512 checksum\n"
              << "  --input <file>          File containing URLs (one per line)\n"
              << "  --max-rate <rate>       Cap speed (e.g. 2m, 500k)\n"
              << "  --retries <n>           Retry failed chunks\n"
              << "  --retry-delay <ms>      Delay between retries\n"
              << "  --timeout <ms>          Transfer timeout\n"
              << "  --connect-timeout <ms>  Connection timeout\n"
              << "  --header <value>        Additional HTTP header (repeatable)\n"
              << "  --user-agent <value>    Custom user agent\n"
              << "  --secure                Enable TLS verification\n"
              << "  --no-resume             Disable resume state\n"
              << "  --help                  Show help" << std::endl;
}

static void signalHandler(int signum) {
    if (global_downloader) {
        std::cout << "\nPausing download safely..." << std::endl;
        global_downloader->Pause();
    }
    curl_global_cleanup();
    exit(signum);
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif

    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (argc < 2) {
        PrintUsage();
        curl_global_cleanup();
        return 1;
    }

    std::vector<std::string> urls;
    std::string output;
    std::string output_dir;
    int threads = 8;
    std::string expected_hash;
    Verifier::HashType hash_type = Verifier::HashType::SHA256;
    std::string hash_name = "SHA-256";
    std::vector<std::string> mirrors;
    size_t max_rate = 0;
    int retries = 2;
    int retry_delay_ms = 500;
    long timeout_ms = 0;
    long connect_timeout_ms = 0;
    std::vector<std::string> headers;
    std::string user_agent;
    bool verify_tls = false;
    bool resume = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            PrintUsage();
            curl_global_cleanup();
            return 0;
        } else if (arg == "--output" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--output-dir" && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        } else if (arg == "--mirrors" && i + 1 < argc) {
            std::string mirrors_arg = argv[++i];
            std::stringstream ss(mirrors_arg);
            std::string segment;
            while (std::getline(ss, segment, ',')) {
                segment = Trim(segment);
                if (!segment.empty()) mirrors.push_back(segment);
            }
        } else if (arg == "--sha256" && i + 1 < argc) {
            expected_hash = argv[++i];
            hash_type = Verifier::HashType::SHA256;
            hash_name = "SHA-256";
        } else if (arg == "--md5" && i + 1 < argc) {
            expected_hash = argv[++i];
            hash_type = Verifier::HashType::MD5;
            hash_name = "MD5";
        } else if (arg == "--sha1" && i + 1 < argc) {
            expected_hash = argv[++i];
            hash_type = Verifier::HashType::SHA1;
            hash_name = "SHA-1";
        } else if (arg == "--sha512" && i + 1 < argc) {
            expected_hash = argv[++i];
            hash_type = Verifier::HashType::SHA512;
            hash_name = "SHA-512";
        } else if (arg == "--input" && i + 1 < argc) {
            std::ifstream input(argv[++i]);
            std::string line;
            while (std::getline(input, line)) {
                line = Trim(line);
                if (!line.empty()) urls.push_back(line);
            }
        } else if (arg == "--max-rate" && i + 1 < argc) {
            max_rate = ParseSize(argv[++i]);
        } else if (arg == "--retries" && i + 1 < argc) {
            retries = std::stoi(argv[++i]);
        } else if (arg == "--retry-delay" && i + 1 < argc) {
            retry_delay_ms = std::stoi(argv[++i]);
        } else if (arg == "--timeout" && i + 1 < argc) {
            timeout_ms = std::stol(argv[++i]);
        } else if (arg == "--connect-timeout" && i + 1 < argc) {
            connect_timeout_ms = std::stol(argv[++i]);
        } else if (arg == "--header" && i + 1 < argc) {
            headers.push_back(argv[++i]);
        } else if (arg == "--user-agent" && i + 1 < argc) {
            user_agent = argv[++i];
        } else if (arg == "--secure") {
            verify_tls = true;
        } else if (arg == "--no-resume") {
            resume = false;
        } else if (!arg.empty() && arg[0] != '-') {
            urls.push_back(arg);
        }
    }

    if (urls.empty()) {
        PrintUsage();
        curl_global_cleanup();
        return 1;
    }

    if (!output.empty() && urls.size() > 1) {
        std::cerr << "--output can only be used with a single URL" << std::endl;
        curl_global_cleanup();
        return 1;
    }

    if (!expected_hash.empty() && urls.size() > 1) {
        std::cerr << "Checksum verification can only be used with a single URL" << std::endl;
        curl_global_cleanup();
        return 1;
    }

    if (!output_dir.empty()) {
        std::filesystem::create_directories(output_dir);
    }

    std::signal(SIGINT, signalHandler);

    DownloadOptions options;
    options.num_threads = threads;
    options.max_rate = max_rate;
    options.retries = retries;
    options.retry_delay_ms = retry_delay_ms;
    options.timeout_ms = timeout_ms;
    options.connect_timeout_ms = connect_timeout_ms;
    options.headers = headers;
    options.user_agent = user_agent;
    options.verify_tls = verify_tls;
    options.resume = resume;

    bool all_success = true;

    for (const auto& url : urls) {
        std::string output_path = output;
        if (output_path.empty()) {
            std::string name = BaseNameFromUrl(url);
            if (!output_dir.empty()) {
                output_path = (std::filesystem::path(output_dir) / name).string();
            } else {
                output_path = name;
            }
        }

        Downloader dl(url, mirrors, output_path, options);
        global_downloader = &dl;

        bool success = dl.Start();
        if (success && !expected_hash.empty()) {
            std::cout << "Verifying " << hash_name << "..." << std::endl;
            if (Verifier::Verify(output_path, expected_hash, hash_type)) {
                std::cout << "Checksum verified: SUCCESS" << std::endl;
            } else {
                std::cout << "Checksum verified: FAILED (File might be corrupted)" << std::endl;
                success = false;
            }
        }

        if (!success) {
            all_success = false;
            break;
        }
    }

    curl_global_cleanup();
    return all_success ? 0 : 1;
}
