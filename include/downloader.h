// include/downloader.h
#pragma once
#include <string>
#include <cstdint>

// Интерфейс загрузчика
class IDownloader {
public:
    virtual ~IDownloader() = default;

    // Загружает URL → outPath с resume, проверкой HTTPS и подписи.
    // Возвращает true при успехе, false иначе.
    virtual bool Download(const std::wstring& url,
                          const std::wstring& outPath,
                          int maxRetries = 3,
                          int backoffSeconds = 2) = 0;
};

// WinHTTP-загрузчик
class WinHttpDownloader : public IDownloader {
public:
    bool Download(const std::wstring& url,
                  const std::wstring& outPath,
                  int maxRetries = 3,
                  int backoffSeconds = 2) override;

private:
    static uint64_t GetFileSize(const std::wstring& path);
    static bool IsSafeOutPath(const std::wstring& outPath);
    static bool VerifyDigitalSignature(const std::wstring& filePath);
};
