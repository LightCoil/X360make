// src/logger.cpp
#include "logger.h"
#include <windows.h>
#include <chrono>
#include <locale>
#include <filesystem>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <clocale>

using namespace std::chrono;
namespace fs = std::filesystem;

void AsyncFileLogger::EnsureConsoleUnicode() {
    std::setlocale(LC_ALL, "");
    _setmode(_fileno(stdout), _O_U16TEXT);
}

bool AsyncFileLogger::OpenFileWithBOM(const std::wstring& path) {
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Logger: CreateFileW failed, err=" << GetLastError() << std::endl;
        return false;
    }
    DWORD written = 0;
    uint8_t bom[] = {0xEF, 0xBB, 0xBF};
    if (!WriteFile(hFile, bom, 3, &written, nullptr) || written != 3) {
        std::wcerr << L"Logger: WriteFile BOM failed, err=" << GetLastError() << std::endl;
        CloseHandle(hFile);
        return false;
    }

    int fd = _open_osfhandle((intptr_t)hFile, _O_APPEND | _O_U8TEXT);
    if (fd < 0) {
        std::wcerr << L"Logger: _open_osfhandle failed" << std::endl;
        CloseHandle(hFile);
        return false;
    }
    FILE* fp = _fdopen(fd, "w, ccs=UTF-8");
    if (!fp) {
        std::wcerr << L"Logger: _fdopen failed" << std::endl;
        _close(fd);
        return false;
    }
    currentFile_.attach(fp);
    currentFile_.flush();
    auto pos = currentFile_.tellp();
    if (pos == -1) {
        fileSize_.store(0, std::memory_order_relaxed);
    } else {
        fileSize_.store(static_cast<size_t>(pos), std::memory_order_relaxed);
    }
    return true;
}

AsyncFileLogger::AsyncFileLogger(const LoggerConfig& config)
    : config_(config)
{
    bool ok = OpenFileWithBOM(config_.filename);
    if (!ok) {
        return;
    }
    if (config_.consoleOutput) {
        EnsureConsoleUnicode();
    }
    running_.store(true, std::memory_order_release);
    try {
        worker_ = std::thread(&AsyncFileLogger::WorkerThread, this);
    } catch (...) {
        running_.store(false, std::memory_order_release);
        currentFile_.close();
    }
}

AsyncFileLogger::~AsyncFileLogger() {
    Close();
}

std::wstring AsyncFileLogger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return L"[DEBUG]";
        case LogLevel::Info:    return L"[INFO]";
        case LogLevel::Warning: return L"[WARN]";
        case LogLevel::Error:   return L"[ERROR]";
        case LogLevel::Fatal:   return L"[FATAL]";
        default:                return L"[UNKNOWN]";
    }
}

std::wstring AsyncFileLogger::Timestamp() {
    auto now = system_clock::now();
    auto t   = system_clock::to_time_t(now);
    tm local_tm;
    localtime_s(&local_tm, &t);
    wchar_t buf[64];
    swprintf_s(buf, L"%04d-%02d-%02d %02d:%02d:%02d",
               local_tm.tm_year + 1900,
               local_tm.tm_mon + 1,
               local_tm.tm_mday,
               local_tm.tm_hour,
               local_tm.tm_min,
               local_tm.tm_sec);
    return buf;
}

void AsyncFileLogger::RotateFileIfNeeded() {
    if (!currentFile_.is_open()) return;
    size_t curSize = fileSize_.load(std::memory_order_acquire);
    if (curSize < config_.maxFileSize) return;

    currentFile_.flush();
    currentFile_.close();

    fs::path oldName = config_.filename;
    std::wstring ts = Timestamp();
    fs::path newName = oldName.string() + L"." + ts + L".log";
    std::error_code ec;
    fs::rename(oldName, newName, ec);
    if (ec) {
        for (int i = 1; i <= 999; ++i) {
            fs::path alt = oldName.string() + L"." + ts + L"_" + std::to_wstring(i) + L".log";
            fs::rename(oldName, alt, ec);
            if (!ec) break;
        }
    }

    if (!OpenFileWithBOM(config_.filename)) {
        running_.store(false, std::memory_order_release);
    }
}

void AsyncFileLogger::WorkerThread() {
    std::atomic_thread_fence(std::memory_order_acquire);
    while (true) {
        std::unique_lock<std::mutex> lock(mtxQueue_);
        cv_.wait(lock, [this]() {
            return !queue_.empty() || !running_.load(std::memory_order_acquire);
        });
        if (!running_.load(std::memory_order_acquire) && queue_.empty()) {
            break;
        }
        std::queue<std::pair<LogLevel, std::wstring>> localQueue;
        std::swap(localQueue, queue_);
        lock.unlock();

        while (!localQueue.empty()) {
            auto [level, msg] = std::move(localQueue.front());
            localQueue.pop();

            std::wstring line = L"[" + Timestamp() + L"] "
                               + LevelToString(level) + L" "
                               + msg + L"\n";
            if (config_.consoleOutput) {
                std::wcout << line;
            }
            if (currentFile_.is_open()) {
                currentFile_ << line;
                // Увеличиваем через атомарный fetch_add
                size_t bytes = line.size() * sizeof(wchar_t);
                fileSize_.fetch_add(bytes, std::memory_order_relaxed);
                RotateFileIfNeeded();
            }
        }
    }

    while (true) {
        std::pair<LogLevel, std::wstring> entry;
        {
            std::lock_guard<std::mutex> lock2(mtxQueue_);
            if (queue_.empty()) break;
            entry = std::move(queue_.front());
            queue_.pop();
        }
        std::wstring line = L"[" + Timestamp() + L"] "
                           + LevelToString(entry.first) + L" "
                           + entry.second + L"\n";
        if (config_.consoleOutput) {
            std::wcout << line;
        }
        if (currentFile_.is_open()) {
            currentFile_ << line;
        }
    }
    if (currentFile_.is_open()) {
        currentFile_.close();
    }
}

void AsyncFileLogger::Log(LogLevel level, const std::wstring& message) {
    if (level < config_.minLevel) return;
    {
        std::lock_guard<std::mutex> lock(mtxQueue_);
        if (queue_.size() >= config_.maxQueueSize) {
            queue_ = std::queue<std::pair<LogLevel, std::wstring>>();
        }
        queue_.emplace(level, message);
    }
    cv_.notify_one();
}

void AsyncFileLogger::Close() {
    running_.store(false, std::memory_order_release);
    cv_.notify_one();
    if (worker_.joinable()) {
        worker_.join();
    }
}
