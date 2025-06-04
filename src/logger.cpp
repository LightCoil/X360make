#include "logger.h"
#include <fstream>
#include <iostream>
#include <windows.h>

static std::wofstream logFile;

void LogInit() {
    logFile.open("build.log", std::ios::out);
    SYSTEMTIME st;
    GetLocalTime(&st);
    logFile << L"[x360make] Build started: "
            << st.wYear << L"-" << st.wMonth << L"-" << st.wDay << L" "
            << st.wHour << L":" << st.wMinute << L"\n";
}

void Log(const std::wstring& msg) {
    std::wcout << msg << L"\n";
    if (logFile.is_open()) logFile << msg << L"\n";
}

void LogClose() {
    logFile << L"[x360make] Build finished\n";
    logFile.close();
}
