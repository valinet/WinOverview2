#include <iostream>
#include <Windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <tlhelp32.h>
#include <Psapi.h>
#include <assert.h>
#include <conio.h>

#define CLASS_NAME L"WinOverview2_master"
#define DUMMY_CLASS_NAME L"WinOverview2_dummy"

#define WM_SETFOREGROUND WM_USER + 1
#define WM_OVERVIEW WM_USER + 2

#define STARTUP_DELAY 2000

#ifdef _DEBUG
#define DEBUG
#endif

HANDLE hThread = NULL;
HANDLE hProcess = NULL;
HMODULE hMod = NULL;
uint64_t hInjection = 0;
DWORD hLibModule = 0;
BOOL isInOverview = FALSE;
BOOL isWinPressed = FALSE;

LONG ExitHandler(LPEXCEPTION_POINTERS p)
{
    HMODULE hKernel32 = NULL;
    FARPROC hAdrFreeLibrary = NULL;

    hThread = CreateRemoteThread(
        hProcess,
        NULL,
        0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>
        ((uint64_t)hMod + (uint64_t)hInjection),
        NULL,
        0,
        NULL
    );
    WaitForSingleObject(
        hThread,
        INFINITE
    );
    GetExitCodeThread(
        hThread,
        &hLibModule
    );
    if (hLibModule)
    {
        wprintf(L"E. Error while unhooking Explorer (%d).\n", hLibModule);
    }
    else
    {
        wprintf(L"Successfully unhooked Explorer.\n");
    }
    TerminateProcess(hProcess, NULL);
    return EXCEPTION_EXECUTE_HANDLER;
}

BOOL WINAPI ConsoleHandler(DWORD signal) {

    if (signal == CTRL_C_EVENT)
    {
        ExitHandler(NULL);
        TerminateProcess(GetCurrentProcess(), NULL);
    }
    return TRUE;
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
        break;
    }
    case WM_OVERVIEW:
    {
        isInOverview = TRUE;
        break;
    }
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI InvokeThread2(
    LPVOID lpParameter
)
{
    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event(0x51, 0, 0, 0);
    keybd_event(0x51, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
    return NULL;
}

void invokeSearch()
{
    TCHAR className[100];
    isInOverview = FALSE;
    CreateRemoteThread(
        hProcess,
        NULL,
        0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>
        ((uint64_t)hMod + (uint64_t)hInjection),
        (LPVOID)-1,
        0,
        NULL
    );
    HWND fWnd;
    do {
        fWnd = GetForegroundWindow();
        GetClassName(fWnd, className, 100);
    } while (!wcsstr(className, TEXT("Windows.UI.Core.CoreW")));
    Sleep(50);
    MONITORINFOEX m;
    m.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(
        MonitorFromWindow(nullptr, MONITOR_DEFAULTTOPRIMARY),
        &m
    );
    RECT rect;
    GetWindowRect(fWnd, &rect);
    SetWindowPos(
        fWnd,
        fWnd,
        m.rcWork.left + (m.rcWork.right - m.rcWork.left) / 2 - (rect.right - rect.left) / 2,
        m.rcWork.top + (m.rcWork.bottom - m.rcWork.top) / 2 - (rect.bottom - rect.top) / 2,
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
    if (!wcscmp(L"Windows.UI.Core.CoreWindow", className))
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
    Sleep(500);
    if (isInOverview && FindWindow(L"MultitaskingViewFrame", NULL) != GetForegroundWindow() && wcscmp(L"Windows.UI.Core.CoreWindow", className))
    {
        invokeSearch();
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
        if (wParam == WM_KEYDOWN && ((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_CAPITAL)
        {
            isInOverview = FALSE;
            return 1;
        }
        else if (wParam == WM_KEYUP && ((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_CAPITAL)
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
        else if (isInOverview && (((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_LWIN || ((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_RWIN))
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
        else if (isInOverview && wParam == WM_KEYDOWN && ((KBDLLHOOKSTRUCT*)lParam)->vkCode >= 0x30 && ((KBDLLHOOKSTRUCT*)lParam)->vkCode <= 0x5A)
        {
            if (!((GetKeyState(VK_CAPITAL) & 0x0001) != 0) &&
                !((GetKeyState(VK_SHIFT) & 0x8000) != 0) &&
                !((GetKeyState(VK_CONTROL) & 0x8000) != 0) &&
                !((GetKeyState(VK_MENU) & 0x8000) != 0) &&
                !isWinPressed &&
                FindWindow(L"MultitaskingViewFrame", NULL) == GetForegroundWindow())
            {
                invokeSearch();
                Sleep(50); //100
            }
        }
        else if (isInOverview)
        {
            if (FindWindow(L"MultitaskingViewFrame", NULL) != GetForegroundWindow())
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
    LPTOP_LEVEL_EXCEPTION_FILTER pExitHandler = NULL;
    HMODULE hKernel32 = NULL;
    FARPROC hAdrLoadLibrary = NULL;
    void* pLibRemote = NULL;
    wchar_t szLibPath[_MAX_PATH];
    wchar_t szTmpLibPath[_MAX_PATH];
    BOOL bResult = FALSE;
    HANDLE snapshot = NULL;
    PROCESSENTRY32 entry;
    DWORD processId = 0;
    HMODULE* hMods = NULL;
    DWORD hModuleArrayInitialBytesInitial = 100 * sizeof(HMODULE);
    DWORD hModuleArrayInitialBytes = hModuleArrayInitialBytesInitial;
    DWORD hModuleArrayBytesNeeded = 0;
    FILE* conout = NULL;
    HMODULE hInjectionDll = NULL;
    FARPROC hInjectionMainFunc = NULL;
    SIZE_T i = 0;
    BOOL bRet = FALSE;
    WNDCLASS wc = { };
    HWND hWnd;
    MSG msg = { };
    HANDLE waitObject = NULL;
    HHOOK hook = NULL;

    if (__argc > 1)
    {
        WNDCLASS wndClass = { 0 };
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
        HWND hWnd = CreateWindowEx(
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
        MSG msg;
        BOOL bRet;
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

    // Step 0: Print debug info
#ifdef DEBUG
    if (!AllocConsole());
    if (freopen_s(&conout, "CONOUT$", "w", stdout));
#endif
    wprintf(L"WinOverview2\n========================\n");

    hook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        hInstance,
        NULL
    );

    // Step 1: Format hook library path
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
    wprintf(
        L"1. Hook Library Path: %s\n", 
        szLibPath
    );

    // Step 2: Get DLL entry point address
    hInjectionDll = LoadLibrary(szLibPath);
    hInjectionMainFunc = GetProcAddress(
        hInjectionDll,
        "main"
    );
    hInjection = reinterpret_cast<DWORD>(hInjectionMainFunc) -
        reinterpret_cast<DWORD>(hInjectionDll);
    FreeLibrary(hInjectionDll);
    wprintf(
        L"2. Hook Library Entry Point: 0x%x\n", 
        hInjection
    );

    // Step 3: Get address of LoadLibrary
    hKernel32 = GetModuleHandle(L"Kernel32");
    assert(hKernel32 != NULL);
    hAdrLoadLibrary = GetProcAddress(
        hKernel32,
        "LoadLibraryW"
    );
    assert(hAdrLoadLibrary != NULL);
    wprintf(
        L"3. LoadLibraryW address: %d\n", 
        hAdrLoadLibrary
    );

    while (TRUE)
    {
        // Step 4: Find Explorer.exe
        do
        {
            entry.dwSize = sizeof(PROCESSENTRY32);
            snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
            if (Process32First(snapshot, &entry) == TRUE)
            {
                while (Process32Next(snapshot, &entry) == TRUE)
                {
                    if (!wcscmp(entry.szExeFile, L"explorer.exe"))
                    {
                        processId = entry.th32ProcessID;
                        hProcess = OpenProcess(
                            PROCESS_ALL_ACCESS,
                            FALSE,
                            processId
                        );
                        assert(hProcess != NULL);
                        wprintf(
                            L"4. Found Explorer, PID: %d\n",
                            processId
                        );
                        break;
                    }
                }
            }
            CloseHandle(snapshot);
            if (processId == 0)
            {
                wprintf(L"4. Waiting for Explorer to start...\n");
                //return -4;
            }
            else
            {
                break;
            }
            Sleep(1000);
        } while (processId == 0);
        Sleep(STARTUP_DELAY);

        // Step 5: Marshall path to library in Explorer's memory
        pLibRemote = VirtualAllocEx(
            hProcess,
            NULL,
            sizeof(szLibPath),
            MEM_COMMIT,
            PAGE_READWRITE
        );
        assert(pLibRemote != NULL);
        bResult = WriteProcessMemory(
            hProcess,
            pLibRemote,
            (void*)szLibPath,
            sizeof(szLibPath),
            NULL
        );
        assert(bResult == TRUE);
        wprintf(L"5. Marshalled library path in Explorer's memory.\n");

        // Step 6: Load library in Explorer
        hThread = CreateRemoteThread(
            hProcess,
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)hAdrLoadLibrary,
            pLibRemote,
            0,
            NULL
        );
        assert(hThread != NULL);
        WaitForSingleObject(
            hThread,
            INFINITE
        );
        GetExitCodeThread(
            hThread,
            &hLibModule
        );
        assert(hLibModule != NULL);
        if (!hLibModule)
        {
            wprintf(L"6. ERROR: Failed to inject library into Explorer.\n");
            return -6;
        }
        wprintf(
            L"6. Successfully injected library into Explorer (%d).\n",
            hLibModule
        );
        // Step 7: Free path from Explorer's memory
        VirtualFreeEx(
            hProcess,
            (LPVOID)pLibRemote,
            0,
            MEM_RELEASE
        );
        wprintf(L"7. Freed path from Explorer's memory.\n");

        // Step 8: Get address of library in Explorer's memory
        hModuleArrayInitialBytes = hModuleArrayInitialBytesInitial;
        hMods = (HMODULE*)calloc(
            hModuleArrayInitialBytes, 1
        );
        assert(hMods != NULL);
        bResult = EnumProcessModulesEx(
            hProcess,
            hMods,
            hModuleArrayInitialBytes,
            &hModuleArrayBytesNeeded,
            LIST_MODULES_ALL
        );
        assert(bResult == TRUE);
        if (hModuleArrayInitialBytes < hModuleArrayBytesNeeded)
        {
            hMods = (HMODULE*)realloc(
                hMods,
                hModuleArrayBytesNeeded
            );
            assert(hMods != NULL);
            hModuleArrayInitialBytes = hModuleArrayBytesNeeded;
            bResult = EnumProcessModulesEx(
                hProcess,
                hMods,
                hModuleArrayInitialBytes,
                &hModuleArrayBytesNeeded,
                LIST_MODULES_ALL
            );
            assert(bResult == TRUE);
        }
        CharLower(szLibPath);
        if (hModuleArrayBytesNeeded / sizeof(HMODULE) == 0)
        {
            i = -1;
        }
        else
        {
            for (i = 0; i < hModuleArrayBytesNeeded / sizeof(HMODULE); ++i)
            {
                bResult = GetModuleFileNameEx(
                    hProcess,
                    hMods[i],
                    szTmpLibPath,
                    _MAX_PATH
                );
                CharLower(szTmpLibPath);
                if (!wcscmp(szTmpLibPath, szLibPath))
                {
                    break;
                }
            }
        }
        if (i == -1)
        {
            wprintf(L"8. ERROR: Cannot find library in Explorer's memory.\n");
            return -8;
        }
        wprintf(
            L"8. Found library in Explorer's memory (%d/%d).\n",
            i,
            hModuleArrayBytesNeeded / sizeof(HMODULE)
        );

        // Step 9: Register and create window
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = WindowProc;
        wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClass(&wc);
        hWnd = CreateWindowEx(
            0,                      // Optional window styles
            CLASS_NAME,          // Window class
            TEXT(""),                    // Window text
            WS_OVERLAPPEDWINDOW,    // Window style
            // Size and position
            100,
            100,
            300,
            300,
            NULL,       // Parent window    
            NULL,       // Menu
            hInstance,  // Instance handle
            NULL      // Additional application data
        );
        if (!hWnd)
        {
            wprintf(L"9. Failed to create message window (%d).\n", GetLastError());
            ExitHandler(NULL);
            return -9;
        }
        ChangeWindowMessageFilter(WM_SETFOREGROUND, MSGFLT_ADD);
        ChangeWindowMessageFilter(WM_OVERVIEW, MSGFLT_ADD);
        wprintf(L"9. Successfully created message window (%d).\n", hWnd);

        // Step 10: Run DLL's main
        hMod = hMods[i];
        hThread = CreateRemoteThread(
            hProcess,
            NULL,
            0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>
                ((uint64_t)hMod + (uint64_t)hInjection),
            hWnd,
            0,
            NULL
        );
        WaitForSingleObject(
            hThread,
            INFINITE
        );
        GetExitCodeThread(
            hThread,
            &hLibModule
        );
        assert(hLibModule == NULL);
        if (hLibModule)
        {
            wprintf(L"10. Error while hooking Explorer (%d).\n", hLibModule);
            return -10;
        }
        wprintf(L"10. Successfully hooked Explorer.\n");
        free(hMods);

        // Step 11: Register exception handler
        if (!pExitHandler)
        {
            pExitHandler = SetUnhandledExceptionFilter(
                reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(ExitHandler)
            );
            if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
                wprintf(
                    L"11. Error registering exception handler.\n"
                );
                return -11;
            }
            wprintf(
                L"11. Registered exception handler (%d).\n",
                pExitHandler
            );
        }

        wprintf(L"Listening for messages...\n");
        while (TRUE)
        {
            BOOL bBreak = FALSE;
            switch (MsgWaitForMultipleObjects(1, &hProcess, FALSE, INFINITE, QS_ALLINPUT))
            {
            case WAIT_OBJECT_0:
            {
                bBreak = TRUE;
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                MSG msg;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
                {
                    if (msg.message == WM_QUIT) 
                    {
                        wprintf(L"Shutting down application...\n");
                        ExitHandler(NULL);
                        return 0;
                    }
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            }
            if (bBreak)
            {
                break;
            }
        }
        DestroyWindow(hWnd);
        MSG msg;
        BOOL bRet;
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
        UnregisterClass(CLASS_NAME, hInstance);

        wprintf(L"Explorer was restarted, rehooking...\n");
        Sleep(5000);
    }

    return 0;
}