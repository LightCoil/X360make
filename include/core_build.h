#pragma once
#include <windows.h>

void StartOnlineBuild(const wchar_t* url);
void StartOfflineBuild(const wchar_t* path);
📄 src/core_build.cpp

#include "core_build.h"
#include "logger.h"
#include "downloader.h"
#include "unzip.h"
#include "packer.h"
#include <shlwapi.h>
#include <filesystem>
#include <cstdlib>

#pragma comment(lib, "shlwapi.lib")
namespace fs = std::filesystem;

void AutoBuild(const std::wstring& folder) {
    Log(L"[MODE] Offline");
    Log(L"[PATH] " + folder);
    fs::create_directory(L"out");

    // 1) Собираем .cpp
    std::wstring includes = L"-Icore -Ijit -Iui";
    for (auto& f : fs::directory_iterator(folder)) {
        if (f.path().extension() == L".cpp") {
            std::wstring obj = L"out\\" + f.path().stem().wstring() + L".o";
            std::wstring cmd = L"embedded\\x360make_gcc.exe -c \"" +
                                f.path().wstring() + L"\" -o \"" + obj +
                                L"\" -ffreestanding -nostdlib " + includes;
            Log(L"[COMPILE] " + cmd);
            if (_wsystem(cmd.c_str()) != 0) {
                Log(L"[ERROR] Compilation failed");
                ShellExecute(NULL, NULL, L"notepad.exe", L"build.log", NULL, SW_SHOWNORMAL);
                exit(1);
            }
        }
    }
    // 2) Линковка в ELF
    std::wstring elf = L"out\\main.elf";
    std::wstring objs;
    for (auto& f : fs::directory_iterator(L"out"))
        if (f.path().extension() == L".o")
            objs += L" \"" + f.path().wstring() + L"\"";
    std::wstring linkCmd = L"embedded\\x360make_ld.exe -T crt\\xex.ld" + objs + L" -o \"" + elf + L"\"";
    Log(L"[LINK] " + linkCmd);
    if (_wsystem(linkCmd.c_str()) != 0) {
        Log(L"[ERROR] Link failed");
        ShellExecute(NULL, NULL, L"notepad.exe", L"build.log", NULL, SW_SHOWNORMAL);
        exit(1);
    }
    // 3) Пакуем в XEX
    std::wstring xex = L"out\\main.xex";
    std::wstring packCmd = L"embedded\\x360make_pack.exe \"" + elf + L"\" \"" + xex + L"\"";
    Log(L"[PACK] " + packCmd);
    if (_wsystem(packCmd.c_str()) != 0) {
        Log(L"[ERROR] Pack failed");
        ShellExecute(NULL, NULL, L"notepad.exe", L"build.log", NULL, SW_SHOWNORMAL);
        exit(1);
    }

    Log(L"[✓] Build successful. Output: " + xex);
}

void StartOfflineBuild(const wchar_t* path) {
    LogInit();
    Start:
    if (PathFileExists((std::wstring(path) + L"\\Makefile").c_str())) {
        Log(L"[MODE] Offline (Makefile)");
        std::wstring cmd = L"mingw32-make -C \"" + std::wstring(path) + L"\"";
        Log(L"[MAKE] " + cmd);
        _wsystem(cmd.c_str());
    } else {
        AutoBuild(path);
    }
    LogClose();
}

void StartOnlineBuild(const wchar_t* url) {
    LogInit();
    Log(L"[MODE] Online");
    std::wstring zip = L"cache\\project.zip";
    std::wstring dir = L"cache\\project";
    fs::create_directory(L"cache");
    bool ok = DownloadZip(std::wstring(url) + L"/archive/refs/heads/main.zip", zip) ||
              DownloadZip(std::wstring(url) + L"/archive/refs/heads/master.zip", zip);
    if (!ok) {
        Log(L"[ERROR] Download failed");
        MessageBox(NULL, L"Download error", L"x360make", MB_ICONERROR);
        exit(1);
    }
    if (!Unzip(zip, dir)) {
        Log(L"[ERROR] Unpack failed");
        MessageBox(NULL, L"Unpack error", L"x360make", MB_ICONERROR);
        exit(1);
    }
    // Ищем первую папку
    for (auto& e : fs::directory_iterator(dir))
        if (e.is_directory()) {
            StartOfflineBuild(e.path().c_str());
            break;
        }
    LogClose();
}
