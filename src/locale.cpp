#include "locale.h"

static LangPack ru = {
    L"Онлайн (GitHub)",
    L"Оффлайн (локальный)",
    L"Ссылка на GitHub:",
    L"Путь к проекту:",
    L"Скомпилировать",
    L"Lightcoil · 2025",
    L"Сборка завершена успешно",
    L"Ошибка загрузки проекта",
    L"Ошибка распаковки архива",
    L"Некорректный ELF-файл"
};

static LangPack en = {
    L"Online (GitHub)",
    L"Offline (local)",
    L"GitHub URL:",
    L"Project path:",
    L"Build",
    L"Lightcoil · 2025",
    L"Build completed successfully",
    L"Failed to download project",
    L"Failed to unpack archive",
    L"Invalid ELF file"
};

LangPack* DetectLanguage() {
    LANGID lang = GetUserDefaultUILanguage();
    if (PRIMARYLANGID(lang) == LANG_RUSSIAN)
        return &ru;
    else
        return &en;
}
📄 include/logger.h

#pragma once
#include <string>

void LogInit();
void Log(const std::wstring& msg);
void LogClose();
