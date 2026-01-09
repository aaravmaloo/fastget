#include "downloader.hpp"
#include "verifier.hpp"
#include "ui.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

using namespace fastget;

Downloader* global_downloader = nullptr;

void signalHandler(int signum) {
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
        std::cout << "Usage: fastget <url> [options]\n"
                  << "Options:\n"
                  << "  --output <path>    Specify output file path\n"
                  << "  --threads <n>      Number of download threads\n"
                  << "  --mirrors <urls>   Comma-separated list of mirror URLs\n"
                  << "  --sha256 <hash>    Verify SHA-256 checksum\n"
                  << "  --md5 <hash>       Verify MD5 checksum\n"
                  << "  --sha1 <hash>      Verify SHA-1 checksum\n"
                  << "  --sha512 <hash>    Verify SHA-512 checksum" << std::endl;
        curl_global_cleanup();
        return 1;
    }

    std::string url = argv[1];
    if (url == "--help" || url == "-h") {
        std::cout << "Usage: fastget <url> [options]\n"
                  << "Options:\n"
                  << "  --output <path>    Specify output file path\n"
                  << "  --threads <n>      Number of download threads\n"
                  << "  --mirrors <urls>   Comma-separated list of mirror URLs\n"
                  << "  --sha256 <hash>    Verify SHA-256 checksum\n"
                  << "  --md5 <hash>       Verify MD5 checksum\n"
                  << "  --sha1 <hash>      Verify SHA-1 checksum\n"
                  << "  --sha512 <hash>    Verify SHA-512 checksum" << std::endl;
        curl_global_cleanup();
        return 0;
    }
    std::string output = "downloaded_file";
    int threads = 8;
    std::string expected_hash = "";
    Verifier::HashType hash_type = Verifier::HashType::SHA256;
    std::string hash_name = "SHA-256";
    std::vector<std::string> mirrors;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--output" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        } else if (arg == "--mirrors" && i + 1 < argc) {
            std::string mirrors_arg = argv[++i];
            std::stringstream ss(mirrors_arg);
            std::string segment;
            while (std::getline(ss, segment, ',')) {
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
        }
    }

    if (output == "downloaded_file") {
        size_t last_slash = url.find_last_of('/');
        if (last_slash != std::string::npos) {
            output = url.substr(last_slash + 1);
        }
    }

    std::signal(SIGINT, signalHandler);

    Downloader dl(url, mirrors, output, threads);
    global_downloader = &dl;

    if (dl.Start()) {
        UI::PrintFooter(true);
        
        if (!expected_hash.empty()) {
            std::cout << "Verifying " << hash_name << "..." << std::endl;
            if (Verifier::Verify(output, expected_hash, hash_type)) {
                std::cout << "Checksum verified: SUCCESS" << std::endl;
            } else {
                std::cout << "Checksum verified: FAILED (File might be corrupted)" << std::endl;
            }
        }
    } else {
        UI::PrintFooter(false, "Could not complete download.");
        curl_global_cleanup();
        return 1;
    }

    curl_global_cleanup();
    return 0;
}
