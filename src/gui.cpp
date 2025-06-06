// src/gui.cpp
#include "gui.h"
#include "locale.h"
#include "version.h"
#include <windows.h>
#include <string>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <commctrl.h>
#include <fmt/core.h>
#include "core_build.h"
#include "logger.h"
#include "downloader.h"
#include "packer.h"

#pragma comment(lib, "Comctl32.lib")

// ID контролов
enum {
    ID_BTN_ONLINE    = 101,
    ID_BTN_OFFLINE   = 102,
    ID_BTN_BUILD     = 103,
    ID_BTN_CANCEL    = 104,
    ID_EDIT_URL      = 201,
    ID_EDIT_PATH     = 202,
    ID_STATIC_STATUS = 301,
    ID_PROGRESS      = 302,
    ID_LIST_LOG      = 401
};

constexpr UINT WM_BUILD_STARTED  = WM_USER + 1;
constexpr UINT WM_BUILD_UPDATE   = WM_USER + 2; // wParam=percent (1–100) или 0 → новая строка лога
constexpr UINT WM_BUILD_FINISHED = WM_USER + 3; // wParam=1 (успех) или 0 (провал)

static HWND hUrlEdit, hPathEdit, hBuildBtn, hCancelBtn;
static HWND hStatusText, hProgressBar, hLogList;
static bool onlineMode = true;

static std::shared_ptr<AsyncFileLogger> logger;
static std::unique_ptr<CoreBuilder> builder;
static std::thread buildThread;

static std::vector<std::wstring> logMessages;
static std::mutex logMutex;

// Добавить запись в лог (вектор + сигнал GUI)
static void AddLogMessage(HWND hWnd, const std::wstring& msg) {
    {
        std::lock_guard<std::mutex> lock(logMutex);
        logMessages.push_back(msg);
    }
    PostMessageW(hWnd, WM_BUILD_UPDATE, 0, 0);
}

static std::atomic<bool> cancelRequested(false);

// Функция потока сборки
static void BuildThreadProc(HWND hWnd, std::wstring input, bool online) {
    PostMessageW(hWnd, WM_BUILD_STARTED, 0, 0);

    auto progressCb = [&](double percent) {
        int p = static_cast<int>(percent);
        PostMessageW(hWnd, WM_BUILD_UPDATE, (WPARAM)p, 0);
    };

    bool success = false;
    if (online) {
        success = builder->StartOnlineBuild(input);
    } else {
        success = builder->StartOfflineBuild(input);
    }
    PostMessageW(hWnd, WM_BUILD_FINISHED, success ? 1 : 0, 0);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        // DPI per-monitor V2
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        UINT dpi = GetDpiForWindow(hWnd);
        float scale = dpi / 96.0f;
        auto S = [&](int x) { return static_cast<int>(x * scale); };

        Locale loc;
        loc.LoadLanguage("en");

        LoggerConfig logCfg;
        logCfg.filename     = L"build.log";
        logCfg.maxFileSize  = 50 * 1024 * 1024;
        logCfg.consoleOutput= false;
        logger = std::make_shared<AsyncFileLogger>(logCfg);
        builder = std::make_unique<CoreBuilder>(
                      logger,
                      std::make_shared<WinHttpDownloader>(),
                      std::make_shared<Packer>());

        CreateWindowW(L"BUTTON", loc.L(L"btnOnline").c_str(),
                      WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON,
                      S(10), S(10), S(150), S(25),
                      hWnd, (HMENU)ID_BTN_ONLINE, nullptr, nullptr);
        CreateWindowW(L"BUTTON", loc.L(L"btnOffline").c_str(),
                      WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
                      S(170), S(10), S(150), S(25),
                      hWnd, (HMENU)ID_BTN_OFFLINE, nullptr, nullptr);

        CreateWindowW(L"STATIC", loc.L(L"lblUrl").c_str(),
                      WS_CHILD | WS_VISIBLE,
                      S(10), S(50), S(100), S(20),
                      hWnd, nullptr, nullptr, nullptr);
        hUrlEdit = CreateWindowW(L"EDIT", L"",
                                 WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                 S(120), S(50), S(300), S(20),
                                 hWnd, (HMENU)ID_EDIT_URL, nullptr, nullptr);

        CreateWindowW(L"STATIC", loc.L(L"lblPath").c_str(),
                      WS_CHILD | WS_VISIBLE,
                      S(10), S(80), S(100), S(20),
                      hWnd, nullptr, nullptr, nullptr);
        hPathEdit = CreateWindowW(L"EDIT", L"",
                                  WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                  S(120), S(80), S(300), S(20),
                                  hWnd, (HMENU)ID_EDIT_PATH, nullptr, nullptr);

        CreateWindowW(L"BUTTON", loc.L(L"btnBuild").c_str(),
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      S(10), S(110), S(100), S(25),
                      hWnd, (HMENU)ID_BTN_BUILD, nullptr, nullptr);
        CreateWindowW(L"BUTTON", loc.L(L"btnCancel").c_str(),
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
                      S(120), S(110), S(100), S(25),
                      hWnd, (HMENU)ID_BTN_CANCEL, nullptr, nullptr);

        hStatusText = CreateWindowW(L"STATIC", L"",
                                   WS_CHILD | WS_VISIBLE,
                                   S(10), S(145), S(410), S(20),
                                   hWnd, (HMENU)ID_STATIC_STATUS, nullptr, nullptr);

        hProgressBar = CreateWindowW(PROGRESS_CLASSW, L"",
                                     WS_CHILD | WS_VISIBLE,
                                     S(10), S(170), S(410), S(20),
                                     hWnd, (HMENU)ID_PROGRESS, nullptr, nullptr);
        SendMessageW(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

        INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
        InitCommonControlsEx(&icex);
        hLogList = CreateWindowW(WC_LISTVIEWW, L"",
                                 WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS,
                                 S(10), S(200), S(410), S(200),
                                 hWnd, (HMENU)ID_LIST_LOG, nullptr, nullptr);
        ListView_SetExtendedListViewStyle(hLogList, LVS_EX_FULLROWSELECT);
        { 
            LVCOLUMNW col = {};
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.pszText = const_cast<wchar_t*>(loc.L(L"colLog").c_str());
            col.cx = S(400);
            ListView_InsertColumn(hLogList, 0, &col);
        }
        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wp)) {
        case ID_BTN_ONLINE:
            onlineMode = true;
            EnableWindow(hBuildBtn, TRUE);
            SetWindowTextW(hStatusText, L"");
            SendMessageW(hProgressBar, PBM_SETPOS, 0, 0);
            ListView_DeleteAllItems(hLogList);
            break;

        case ID_BTN_OFFLINE:
            onlineMode = false;
            EnableWindow(hBuildBtn, TRUE);
            SetWindowTextW(hStatusText, L"");
            SendMessageW(hProgressBar, PBM_SETPOS, 0, 0);
            ListView_DeleteAllItems(hLogList);
            break;

        case ID_BTN_BUILD: {
            wchar_t bufUrl[1024], bufPath[1024];
            GetWindowTextW(hUrlEdit, bufUrl, 1024);
            GetWindowTextW(hPathEdit, bufPath, 1024);
            std::wstring input = onlineMode ? bufUrl : bufPath;
            if (input.empty()) {
                MessageBoxW(hWnd, loc.L(L"errEmpty").c_str(), loc.L(L"title").c_str(), MB_OK);
                break;
            }
            EnableWindow(hBuildBtn, FALSE);
            EnableWindow(hCancelBtn, TRUE);
            logMessages.clear();
            cancelRequested.store(false, std::memory_order_release);
            builder->CancelBuild();

            buildThread = std::thread([hWnd, input, onlineMode]() {
                BuildThreadProc(hWnd, input, onlineMode);
            });
            break;
        }

        case ID_BTN_CANCEL:
            cancelRequested.store(true, std::memory_order_acquire);
            builder->CancelBuild();
            SetWindowTextW(hStatusText, L"Cancel requested...");
            EnableWindow(hCancelBtn, FALSE);
            break;
        }
        return 0;
    }

    case WM_BUILD_STARTED:
        SetWindowTextW(hStatusText, L"Build started...");
        return 0;

    case WM_BUILD_UPDATE: {
        if (wp != 0) {
            int percent = static_cast<int>(wp);
            SendMessageW(hProgressBar, PBM_SETPOS, percent, 0);
            wchar_t buf[128];
            swprintf_s(buf, L"Progress: %d%%", percent);
            SetWindowTextW(hStatusText, buf);
        } else {
            std::lock_guard<std::mutex> lock(logMutex);
            int idx = (int)logMessages.size() - 1;
            if (idx >= 0 && !cancelRequested.load(std::memory_order_acquire)) {
                SendMessage(hLogList, WM_SETREDRAW, FALSE, 0);

                std::wstring copy = logMessages[idx];
                LVITEMW lvi{};
                lvi.mask   = LVIF_TEXT;
                lvi.iItem  = idx;
                lvi.pszText= const_cast<wchar_t*>(copy.c_str());
                ListView_InsertItem(hLogList, &lvi);
                ListView_EnsureVisible(hLogList, idx, FALSE);

                SendMessage(hLogList, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(hLogList, nullptr, TRUE);
            }
        }
        return 0;
    }

    case WM_BUILD_FINISHED: {
        bool success = (wp != 0);
        if (success) {
            SetWindowTextW(hStatusText, L"Build completed successfully.");
            SendMessageW(hProgressBar, PBM_SETPOS, 100, 0);
        } else {
            SetWindowTextW(hStatusText, L"Build failed or cancelled.");
            SendMessageW(hProgressBar, PBM_SETPOS, 0, 0);
        }
        EnableWindow(hBuildBtn, TRUE);
        EnableWindow(hCancelBtn, FALSE);
        return 0;
    }

    case WM_DESTROY:
        cancelRequested.store(true, std::memory_order_acquire);
        builder->CancelBuild();
        if (buildThread.joinable()) {
            buildThread.join();
        }
        if (logger) {
            logger->Close();
            logger.reset();
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wp, lp);
}

int RunGUI(HINSTANCE hInstance, int nCmdShow) {
    WNDCLASSW wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"x360make";
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowW(L"x360make",
                              L"x360make " SE_VERSION,
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) {
        return -1;
    }
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
