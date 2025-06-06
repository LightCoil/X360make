// include/locale.h
#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

// Потокобезопасная карта локализации: ключ/значение — wide string
using LangMap = std::unordered_map<std::wstring, std::wstring>;

class Locale {
public:
    Locale() = default;
    virtual ~Locale() = default;   // виртуальный деструктор для наследования

    // Загружает "lang/lang_<code>.json". Возвращает true при успехе, false — иначе.
    bool LoadLanguage(const std::string& lang_code);

    // Возвращает перевод для key. Если key пуст или не найден, возвращает сам key.
    const std::wstring& L(const std::wstring& key);

protected:
    // Проверяет, что lang_code состоит только из [A-Za-z0-9_-] и длина ≤16
    static bool IsSafeLangCode(const std::string& code);

    // Безопасная конвертация UTF-8 → UTF-16 (MB_ERR_INVALID_CHARS)
    static bool Utf8ToWStringSafe(const std::string& utf8, std::wstring& out);

    // Функции, возвращающие ссылки на function-local статические объекты:
    static LangMap& GetLangMap();
    static std::mutex& GetMutex();
};
