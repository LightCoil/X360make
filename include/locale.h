#pragma once
#include <windows.h>

struct LangPack {
    const wchar_t* btnOnline;
    const wchar_t* btnOffline;
    const wchar_t* lblUrl;
    const wchar_t* lblPath;
    const wchar_t* btnBuild;
    const wchar_t* lblFooter;
    const wchar_t* msgSuccess;
    const wchar_t* msgErrorDownload;
    const wchar_t* msgErrorUnpack;
    const wchar_t* msgErrorElf;
};

LangPack* DetectLanguage();
