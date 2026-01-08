#include "downloader.hpp"
#include "verifier.hpp"
#include "ui.hpp"
#include <iostream>
#include <string>
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
        std::cout << "Usage: fastget <url> [--output <path>] [--threads <n>] [--sha256 <hash>]" << std::endl;
        curl_global_cleanup();
        return 1;
    }

    std::string url = argv[1];
    if (url == "--help" || url == "-h") {
        std::cout << "Usage: fastget <url> [--output <path>] [--threads <n>] [--sha256 <hash>]" << std::endl;
        curl_global_cleanup();
        return 0;
    }
    std::string output = "downloaded_file";
    int threads = 8;
    std::string expected_hash = "";

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--output" && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        } else if (arg == "--sha256" && i + 1 < argc) {
            expected_hash = argv[++i];
        }
    }

    if (output == "downloaded_file") {
        size_t last_slash = url.find_last_of('/');
        if (last_slash != std::string::npos) {
            output = url.substr(last_slash + 1);
        }
    }

    std::signal(SIGINT, signalHandler);

    Downloader dl(url, output, threads);
    global_downloader = &dl;

    if (dl.Start()) {
        UI::PrintFooter(true);
        
        if (!expected_hash.empty()) {
            std::cout << "Verifying SHA-256..." << std::endl;
            if (Verifier::Verify(output, expected_hash)) {
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
