# fastget

A high-performance, cross-platform download accelerator built with C++20, libcurl, and OpenSSL.

## Features
- **Parallel Downloads**: Splits files into chunks and downloads them concurrently.
- **Adaptive Chunk Sizing**: Automatically adjusts chunk size and concurrency based on network conditions (ideal for flaky Wi-Fi).
- **Resume Capability**: Resumes interrupted downloads using HTTP Range requests.
- **SHA-256 Verification**: Built-in integrity checks using OpenSSL.
- **Clean UX**: Minimal, beautiful terminal progress bars with speed and ETA.
- **Single Binary**: No scripting or heavy dependencies.

## Building (Windows)
1. Ensure you have [CMake](https://cmake.org/download/), [CURL](https://curl.se/), and [OpenSSL](https://www.openssl.org/) installed.
2. Open a terminal in the project root (`fastget/`).
3. Run the following commands:
```powershell
# Create a fresh build directory
Remove-Item build -Recurse -Force -ErrorAction SilentlyContinue
mkdir build
cd build

# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# The binary will be at:
# fastget/build/fastget.exe
```
4. To run it with all DLLs, use the pre-organized folder:
```powershell
./bin/fastget.exe <url>
```

## Building (Linux)
```bash
sudo apt install libcurl4-openssl-dev libssl-dev cmake g++
mkdir build
cd build
cmake ..
make -j$(nproc)
cp fastget ../bin/ # If on Linux
```

## Usage
```bash
./bin/fastget <url> [--output <filename>] [--threads <count>] [--sha256 <hash>]
```

Example:
```bash
./bin/fastget https://example.com/largefile.zip --output local.zip --threads 16
```

## Architecture
- **Downloader**: Orchestrates threads and lifecycle.
- **ChunkManager**: Manages chunk distribution and adaptive logic.
- **NetworkLayer**: Libcurl wrapper for HTTP(S) range requests.
- **FileWriter**: Thread-safe concurrent file writing.
- **Verifier**: SHA-256 hash calculation.
- **UI**: Terminal progress tracking.
