// src/locale.cpp
#include "locale.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <sstream>
#include <windows.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Ограничение длины lang_code
bool Locale::IsSafeLangCode(const std::string& code) {
    if (code.empty() || code.size() > 16) return false;
    for (char c : code) {
        if (!((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
               c == '_' || c == '-'))
        {
            return false;
        }
    }
    return true;
}

bool Locale::Utf8ToWStringSafe(const std::string& utf8, std::wstring& out) {
    if (utf8.empty()) {
        out.clear();
        return true;
    }
    // Отбрасываем BOM (0xEF 0xBB 0xBF) при наличии
    size_t start = 0;
    if (utf8.size() >= 3 &&
        (unsigned char)utf8[0] == 0xEF &&
        (unsigned char)utf8[1] == 0xBB &&
        (unsigned char)utf8[2] == 0xBF)
    {
        start = 3;
    }
    int needed = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8.data() + start, (int)utf8.size() - (int)start,
        nullptr, 0
    );
    if (needed <= 0) {
        out.clear();
        return false;
    }
    out.resize(needed);
    int ret = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        utf8.data() + start, (int)utf8.size() - (int)start,
        &out[0], needed
    );
    if (ret <= 0) {
        out.clear();
        return false;
    }
    return true;
}

LangMap& Locale::GetLangMap() {
    static LangMap g_lang_map_;
    return g_lang_map_;
}

std::mutex& Locale::GetMutex() {
    static std::mutex g_mutex_;
    return g_mutex_;
}

bool Locale::LoadLanguage(const std::string& lang_code) {
    std::lock_guard<std::mutex> lock(GetMutex());

    auto& g_lang_map = GetLangMap();
    g_lang_map.clear();

    if (!IsSafeLangCode(lang_code)) {
        return false;
    }
    const std::string baseDir = "lang";
    try {
        fs::path basePath = fs::path(baseDir);
        if (!fs::exists(basePath) || !fs::is_directory(basePath)) {
            return false;
        }
    } catch (...) {
        return false;
    }

    std::string filename = baseDir + "/lang_" + lang_code + ".json";
    std::error_code ec;
    fs::path candidate = fs::path(filename);
    fs::path pCanonical;
    try {
        pCanonical = fs::weakly_canonical(candidate, ec);
    } catch (...) {
        return false;
    }
    if (ec) return false;

    fs::path baseCan;
    try {
        baseCan = fs::weakly_canonical(fs::path(baseDir), ec);
    } catch (...) {
        return false;
    }
    if (ec) return false;

    std::string sBase = baseCan.string();
    std::string sCan  = pCanonical.string();
    if (sCan.rfind(sBase, 0) != 0) {
        return false;
    }

    if (!fs::exists(pCanonical)) {
        fs::path fallback = baseCan / "lang_en.json";
        if (!fs::exists(fallback)) {
            return false;
        }
        pCanonical = fallback;
    }

    std::ifstream file(pCanonical, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    json j;
    try {
        j = json::parse(content);
    } catch (const std::exception&) {
        return false;
    }

    LangMap newMap;
    for (auto& item : j.items()) {
        const auto& keyUtf8 = item.key();
        if (keyUtf8.empty()) continue;

        std::string valUtf8;
        try {
            valUtf8 = item.value().get<std::string>();
        } catch (...) {
            continue;
        }

        std::wstring keyW, valW;
        if (!Utf8ToWStringSafe(keyUtf8, keyW)) continue;
        if (!Utf8ToWStringSafe(valUtf8, valW)) continue;
        newMap.emplace(std::move(keyW), std::move(valW));
    }

    GetLangMap().swap(newMap);
    return true;
}

const std::wstring& Locale::L(const std::wstring& key) {
    if (key.empty()) {
        static const std::wstring empty = L"";
        return empty;
    }
    std::lock_guard<std::mutex> lock(GetMutex());
    auto& g_lang_map = GetLangMap();
    auto it = g_lang_map.find(key);
    if (it != g_lang_map.end()) {
        return it->second;
    }
    // Возвращаем сохранённый fallback, чтобы не возвращать ссылку на временный объект
    static const std::wstring fallback_empty = L"";
    static std::wstring fallback_key;
    fallback_key = key;
    return fallback_key;
}
