#include "downloader.h"
#include <windows.h>
#include <wininet.h>
#include <fstream>
#pragma comment(lib, "wininet.lib")

bool DownloadZip(const std::wstring& url, const std::wstring& outPath) {
    HINTERNET hNet = InternetOpen(L"x360make", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hNet) return false;
    HINTERNET hUrl = InternetOpenUrl(hNet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) { InternetCloseHandle(hNet); return false; }

    std::ofstream out(outPath, std::ios::binary);
    char buf[4096]; DWORD read;
    while (InternetReadFile(hUrl, buf, sizeof(buf), &read) && read) {
        out.write(buf, read);
    }
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);
    return true;
}
