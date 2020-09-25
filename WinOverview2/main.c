#include <valinet/hooking/exeinject.h>
#include <stdio.h>
#include <stdint.h>
#include <Windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#define CLASS_NAME L"WinOverview2_master"
#define DUMMY_CLASS_NAME L"WinOverview2_dummy"

#define WM_SETFOREGROUND WM_USER + 1
#define WM_OVERVIEW WM_USER + 2

#define STARTUP_DELAY 2000
#define ACTIVATE_CHECK_DELAY 100

#ifdef _DEBUG
#define DEBUG
#endif

HANDLE hProcess = NULL;
HMODULE hMod = NULL;
LPVOID hInjection = NULL;
BOOL isInOverview = FALSE;
BOOL isWinPressed = FALSE;
HHOOK hHook = NULL;
HWND hWnd = NULL;

DWORD ExitHandler(LPVOID _null)
{
    if (hHook)
    {
        UnhookWindowsHookEx(hHook);
    }
    return ERROR_SUCCESS;
}

LRESULT CALLBACK WindowProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_SETFOREGROUND:
    {
        HWND hwnd = (HWND)wParam;
        if (IsIconic(hwnd))
        {
            ShowWindow(hwnd, SW_RESTORE);
        }
        SetForegroundWindow(hwnd);
        BringWindowToTop(hwnd);
        return 0;
    }
    case WM_OVERVIEW:
    {
        isInOverview = TRUE;
        return 0;
    }
    }
    return VnWindowProc(
        hWnd, 
        uMsg, 
        wParam, 
        lParam
    );
}

void InvokeSearch()
{
    TCHAR className[100];
    isInOverview = FALSE;
    CreateRemoteThread(
        hProcess,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)((uint64_t)hMod + (uint64_t)hInjection),
        (LPVOID)-1,
        0,
        NULL
    );
    HWND fWnd;
    do {
        fWnd = GetForegroundWindow();
        GetClassName(fWnd, className, 100);
    } while (!wcsstr(
        className, 
        TEXT("Windows.UI.Core.CoreW")
    ));
    Sleep(50);
    MONITORINFOEX m;
    m.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(
        MonitorFromWindow(
            NULL, 
            MONITOR_DEFAULTTOPRIMARY),
        &m
    );
    RECT rect;
    GetWindowRect(fWnd, &rect);
    SetWindowPos(
        fWnd,
        fWnd,
        m.rcWork.left + 
            (m.rcWork.right - m.rcWork.left) / 2 - 
            (rect.right - rect.left) / 2,
        m.rcWork.top + 
            (m.rcWork.bottom - m.rcWork.top) / 2 - 
            (rect.bottom - rect.top) / 2,
        0,
        0,
        SWP_NOZORDER | SWP_NOSIZE
    );

}

DWORD WINAPI InvokeThread(
    LPVOID lpParameter
)
{
    TCHAR className[100];
    GetClassName(
        GetForegroundWindow(), 
        className, 
        100
    );
    if (!wcscmp(
        L"Windows.UI.Core.CoreWindow", 
        className
    ))
    {
        keybd_event(VK_ESCAPE, 0, 0, 0);
        keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0);
        Sleep(100);
        return NULL;
    }
    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event(0x43, 0, 0, 0);
    keybd_event(0x43, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
    Sleep(ACTIVATE_CHECK_DELAY);
    if (isInOverview && 
        FindWindow(
            L"MultitaskingViewFrame", 
            NULL
        ) != GetForegroundWindow() && 
        wcscmp(
            L"Windows.UI.Core.CoreWindow", 
            className
        )
    )
    {
        InvokeSearch();
    }
    return NULL;
}

LRESULT CALLBACK LowLevelKeyboardProc(
    _In_ int    nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    if (nCode >= 0)
    {
        if (wParam == WM_KEYDOWN && 
            ((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_CAPITAL)
        {
            isInOverview = FALSE;
            return 1;
        }
        else if (wParam == WM_KEYUP && 
            ((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_CAPITAL)
        {
            isInOverview = FALSE;
            CreateThread(
                NULL,
                NULL,
                InvokeThread,
                NULL,
                NULL,
                NULL
            );
            return 1;
        }
        else if (isInOverview && 
            (
                ((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_LWIN || 
                ((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_RWIN
            )
        )
        {
            if (wParam == WM_KEYDOWN)
            {
                isWinPressed = TRUE;
            }
            else if (wParam == WM_KEYUP)
            {
                isWinPressed = FALSE;
            }
        }
        else if (isInOverview && 
            wParam == WM_KEYDOWN && 
            ((KBDLLHOOKSTRUCT*)lParam)->vkCode >= 0x30 && 
            ((KBDLLHOOKSTRUCT*)lParam)->vkCode <= 0x5A
        )
        {
            if (!((GetKeyState(VK_CAPITAL) & 0x0001) != 0) &&
                !((GetKeyState(VK_SHIFT) & 0x8000) != 0) &&
                !((GetKeyState(VK_CONTROL) & 0x8000) != 0) &&
                !((GetKeyState(VK_MENU) & 0x8000) != 0) &&
                !isWinPressed &&
                FindWindow(
                    L"MultitaskingViewFrame", 
                    NULL
                ) == GetForegroundWindow())
            {
                InvokeSearch();
                Sleep(50); //100
            }
        }
        else if (isInOverview)
        {
            if (FindWindow(
                L"MultitaskingViewFrame", 
                NULL
            ) != GetForegroundWindow())
            {
                isInOverview = FALSE;
            }
        }
    }
    return CallNextHookEx(
        NULL,
        nCode,
        wParam,
        lParam
    );
}

int WINAPI wWinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    PWSTR pCmdLine, 
    int nCmdShow
)
{
    FILE* conout;
    wchar_t szLibPath[_MAX_PATH];
    WNDCLASS wndClass = { 0 };
    HWND hWnd = NULL;
    MSG msg = { 0 };
    BOOL bRet = FALSE;

    if (__argc > 1)
    {
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc = WindowProc;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = 0;
        wndClass.hInstance = hInstance;
        wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = DUMMY_CLASS_NAME;
        RegisterClass(&wndClass);
        hWnd = CreateWindowEx(
            WS_EX_LAYERED,
            DUMMY_CLASS_NAME,
            L"",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL,
            NULL,
            hInstance,
            NULL
        );
        SetLayeredWindowAttributes(
            hWnd,
            NULL,
            0,
            LWA_ALPHA
        );
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
        SetForegroundWindow(hWnd);
        while ((bRet = GetMessage(
            &msg,
            NULL,
            0,
            0)) != 0)
        {
            // An error occured
            if (bRet == -1)
            {
                break;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        return 0;
    }

    HWND prev = FindWindow(CLASS_NAME, NULL);
    if (prev)
    {
        SendMessage(
            prev,
            WM_CLOSE,
            0,
            0
        );
        return 0;
    }
#ifdef _DEBUG
    if (!AllocConsole());
    if (freopen_s(
        &conout, 
        "CONOUT$", 
        "w", 
        stdout
    ));
#endif
    hHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        hInstance,
        NULL
    );
    printf("WinOverview2\n===============\n");
    GetModuleFileName(
        GetModuleHandle(NULL),
        szLibPath,
        _MAX_PATH
    );
    PathRemoveFileSpec(szLibPath);
    lstrcat(
        szLibPath,
        L"\\WinOverview2Library.dll"
    );
    int messages[2] = { WM_SETFOREGROUND, WM_OVERVIEW };
    return VnInjectAndMonitorProcess(
        szLibPath,
        _MAX_PATH,
        "main",
        TEXT("explorer.exe"),
        CLASS_NAME,
        NULL,
        hInstance,
        stdout,
        5000,
        WindowProc,
        TRUE,
        1000,
        STARTUP_DELAY,
        ExitHandler,
        &hProcess,
        &hMod,
        &hInjection,
        messages,
        2,
        &hWnd,
        &hWnd
    );
}