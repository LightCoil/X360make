#include "gui.h"
#include "locale.h"
#include "core_build.h"
#include "version.h"

static HWND hUrlEdit, hPathEdit;
static bool onlineMode = true;
static LangPack* lang;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        lang = DetectLanguage();
        CreateWindow(L"BUTTON", lang->btnOnline, WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
                     10,10,150,25, hWnd, (HMENU)101, NULL,NULL);
        CreateWindow(L"BUTTON", lang->btnOffline, WS_CHILD|WS_VISIBLE|BS_RADIOBUTTON,
                     170,10,150,25, hWnd, (HMENU)102, NULL,NULL);
        CreateWindow(L"STATIC", lang->lblUrl, WS_CHILD|WS_VISIBLE,
                     10,50,100,20,hWnd,NULL,NULL,NULL);
        hUrlEdit = CreateWindow(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER,
                     120,50,300,20,hWnd,NULL,NULL,NULL);
        CreateWindow(L"STATIC", lang->lblPath, WS_CHILD|WS_VISIBLE,
                     10,80,100,20,hWnd,NULL,NULL,NULL);
        hPathEdit = CreateWindow(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER,
                     120,80,300,20,hWnd,NULL,NULL,NULL);
        CreateWindow(L"BUTTON", lang->btnBuild, WS_CHILD|WS_VISIBLE,
                     10,120,100,30,hWnd,(HMENU)103,NULL,NULL);
        CreateWindow(L"STATIC", lang->lblFooter, WS_CHILD|WS_VISIBLE,
                     10,160,200,20,hWnd,NULL,NULL,NULL);
    } return 0;
    case WM_COMMAND:
        if (LOWORD(wp)==101) onlineMode=true;
        if (LOWORD(wp)==102) onlineMode=false;
        if (LOWORD(wp)==103) {
            wchar_t buf[512];
            GetWindowText(hUrlEdit, buf,512);
            GetWindowText(hPathEdit, buf,512);
            if (onlineMode) StartOnlineBuild(buf);
            else           StartOfflineBuild(buf);
            MessageBox(NULL, lang->msgSuccess, L"x360make", MB_OK);
        }
        break;
    case WM_DESTROY: PostQuitMessage(0); break;
    }
    return DefWindowProc(hWnd,msg,wp,lp);
}

int RunGUI(HINSTANCE hInstance, int nCmdShow) {
    WNDCLASS wc = {0}; wc.lpfnWndProc=WndProc; wc.hInstance=hInstance; wc.lpszClassName=L"x360make";
    RegisterClass(&wc);
    HWND hWnd = CreateWindow(L"x360make", L"x360make " X360MAKE_VERSION, WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,CW_USEDEFAULT,450,240,NULL,NULL,hInstance,NULL);
    ShowWindow(hWnd,nCmdShow);
    MSG msg;
    while (GetMessage(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
