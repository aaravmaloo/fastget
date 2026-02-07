cmake_minimum_required(VERSION 3.16)
project(fastget LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif()

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(src)

set(SOURCES
    src/main.cpp
    src/downloader.cpp
    src/chunk_manager.cpp
    src/network.cpp
    src/file_writer.cpp
    src/verifier.cpp
    src/ui.cpp
    src/resume_state.cpp
)

add_executable(fastget ${SOURCES})

target_link_libraries(fastget 
    PRIVATE 
    CURL::libcurl 
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
)

if(WIN32)
    target_compile_definitions(fastget PRIVATE NOMINMAX)
endif()
