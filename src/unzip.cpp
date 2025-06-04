#include "unzip.h"
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

// Простая оболочка: используем powershell Expand-Archive
bool Unzip(const std::wstring& zipPath, const std::wstring& outDir) {
    std::wstring cmd = L"powershell -Command \"Expand-Archive -Force '"
                      + zipPath + L"' '" + outDir + L"'\"";
    return _wsystem(cmd.c_str()) == 0;
}
