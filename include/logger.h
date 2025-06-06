// include/logger.h
#pragma once
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <cstddef>
#include <fmt/core.h>

// Уровни логирования
enum class LogLevel { Debug, Info, Warning, Error, Fatal };

// Конфигурация логгера
struct LoggerConfig {
    std::wstring filename;            // имя файла (может быть UNC)
    size_t maxFileSize      = 10*1024*1024;  // ротируем при достижении 10 МБ
    bool consoleOutput      = true;   // выводить ли в консоль
    LogLevel minLevel       = LogLevel::Info;
    size_t maxQueueSize     = 10000;  // ограничение очереди логов
};

// Асинхронный логгер с ротацией
class AsyncFileLogger {
public:
    explicit AsyncFileLogger(const LoggerConfig& config);
    ~AsyncFileLogger();

    // Добавить запись (потокобезопасно)
    void Log(LogLevel level, const std::wstring& message);

    // Закрыть логгер (ждёт завершения потока)
    void Close();

private:
    void WorkerThread();          // основной рабочий поток
    void RotateFileIfNeeded();    // проверка необходимости ротации
    static std::wstring LevelToString(LogLevel level);
    static std::wstring Timestamp();

    LoggerConfig config_;
    std::wofstream currentFile_;
    std::atomic<size_t> fileSize_{0};
    std::mutex mtxQueue_;
    std::condition_variable cv_;
    std::queue<std::pair<LogLevel, std::wstring>> queue_;
    std::thread worker_;
    std::atomic<bool> running_{false};

    // Вспомогательный: открывает файл и пишет BOM
    bool OpenFileWithBOM(const std::wstring& path);

    // Устанавливает локаль консоли для корректного вывода wide‐текстов
    static void EnsureConsoleUnicode();
};
