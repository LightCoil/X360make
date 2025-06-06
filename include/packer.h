// include/packer.h
#pragma once
#include <string>
#include <functional>

// Интерфейс упаковщика
class IPacker {
public:
    virtual ~IPacker() = default;
    // Pack: elfPath → xexPath. progressCallback(percent) может быть nullptr.
    // outStdout/outStderr собирают вывод процесса. Возвращает true при успехе.
    virtual bool Pack(const std::wstring& elfPath,
                      const std::wstring& xexPath,
                      std::function<void(double)> progressCallback,
                      std::wstring& outStdout,
                      std::wstring& outStderr) = 0;
};

class Packer : public IPacker {
public:
    bool Pack(const std::wstring& elfPath,
              const std::wstring& xexPath,
              std::function<void(double)> progressCallback,
              std::wstring& outStdout,
              std::wstring& outStderr) override;
};
