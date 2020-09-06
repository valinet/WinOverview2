#include <iostream>
#include <Windows.h>
#include <funchook.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

#define DUMMY_CLASS_NAME L"WinOverview2_dummy"
#define WPARAM_CORTANA 33
#define WM_SETFOREGROUND WM_USER + 1
#define WM_OVERVIEW WM_USER + 2

funchook_t* funchook = NULL;
HMODULE hModule = NULL;
BOOL first = TRUE;
void* CSnapAssistControllerBase = NULL;
HWND messageWindow = NULL;

typedef struct _CRECT
{
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} CRECT;
static HRESULT(*OnMessageFunc)(
    void* _this,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );
static HRESULT(*ShowSnapAssistFunc)(
    void* _this,
    void* IApplicationViewCollection,
    CRECT* rect,
    uint32_t SNAP_ASSIST_VIEW_FLAGS
    );
static HWND(*GetWindowForView)(
    void* appView
    );
static HRESULT(*ItemSelectedFunc)(
    void* _this,
    void* appView,
    CRECT* rect
    );
static HRESULT(*LaunchCortanaApp)(
    const wchar_t* pszArgument,
    uint32_t launchType,
    bool fAllowSetForegroundWindow
    );

HRESULT OnMessageHook(
    void* _this,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    /*TCHAR buffer[100];
    swprintf_s(buffer, L"%d", wParam);
    MessageBox(0, buffer, L"", 0);*/
    if (wParam == WPARAM_CORTANA)
    {
        if (CSnapAssistControllerBase != NULL)
        {
            HWND hwnd = GetForegroundWindow();
            TCHAR className[100];
            GetClassName(hwnd, className, 100);
            if (wcscmp(className, L"MultitaskingViewFrame"))
            {
                SendMessage(
                    messageWindow,
                    WM_OVERVIEW,
                    NULL,
                    NULL
                );
                MONITORINFOEX m;
                m.cbSize = sizeof(MONITORINFOEX);
                GetMonitorInfo(
                    MonitorFromWindow(nullptr, MONITOR_DEFAULTTOPRIMARY),
                    &m
                );
                CRECT rect;
                rect.left = m.rcWork.left;
                rect.top = m.rcWork.top;
                rect.right = m.rcWork.right;
                rect.bottom = m.rcWork.bottom;
                ShowSnapAssistFunc(
                    CSnapAssistControllerBase,
                    NULL,
                    &rect,
                    NULL
                );
            }
            else
            {
                keybd_event(VK_ESCAPE, 0, 0, 0);
                keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0);
            }
        }
        else
        {
            /*MessageBox(
                NULL,
                L"To enable this feature, snap any window to the side of the screen.",
                L"Windows Explorer",
                0
            );*/
        }

        return 0;
    }

    return OnMessageFunc(
        _this,
        msg,
        wParam,
        lParam
    );
}

HRESULT ShowSnapAssistHook(
    void* _this,
    void* IApplicationViewCollection,
    CRECT* rect,
    uint32_t SNAP_ASSIST_VIEW_FLAGS
)
{   
    CSnapAssistControllerBase = _this;
    
    if (first)
    {
        first = FALSE;
        return 0;
    }
    return ShowSnapAssistFunc(
        _this,
        IApplicationViewCollection,
        rect,
        SNAP_ASSIST_VIEW_FLAGS
    );
}

HRESULT ItemSelectedHook(
    void* _this,
    void* appView,
    CRECT* rect
)
{
    HWND hWnd = GetWindowForView(appView);
    SendMessage(
        messageWindow,
        WM_SETFOREGROUND,
        (WPARAM)hWnd,
        NULL
    );
    return ItemSelectedFunc(
        _this,
        appView,
        rect
    );
}

DWORD WINAPI KeysThread(
    LPVOID lpParameter
)
{
    HWND hWnd;
    do {
        hWnd = FindWindow(DUMMY_CLASS_NAME, NULL);
    } while (hWnd == NULL);
    Sleep(100);
    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event(VK_LEFT, 0, 0, 0);
    keybd_event(VK_LEFT, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
    Sleep(1000);
    keybd_event(VK_ESCAPE, 0, 0, 0);
    keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0);
    PostMessage(hWnd, WM_CLOSE, 0, 0);
    Sleep(500);
    PostMessage(FindWindow(DUMMY_CLASS_NAME, NULL), WM_CLOSE, 0, 0);
    return 0;
}

__declspec(dllexport) DWORD WINAPI main(
    _In_ LPVOID lpParameter
)
{
    FILE* conout;
    int rv;
    if (funchook && (int)lpParameter == -1)
    {
        LaunchCortanaApp(
            L"StartMode=Proactive&QuerySource=TextShortcutKey",
            0,
            TRUE
        );
        return 0;
    }
    if (!funchook)
    {
        messageWindow = (HWND)lpParameter;

        funchook = funchook_create();

        //HANDLE hPcs = GetModuleHandle(L"twinui.pcshell.dll");
        HANDLE hTw = GetModuleHandle(L"twinui.dll");

        //GetMultitaskingViewFunc = (long(*)(void*, uint32_t, void*, void*))((uintptr_t)hPcs + (uintptr_t)0x35D2C0);

        //TriggerSnapAssistFromApplicationArrangedNotificationFunc = (HRESULT(*)(void*, void*))((uintptr_t)hTw + (uintptr_t)0x386C70);

        //GetMTVHostKindFunc = (uint32_t(*)(uint32_t))((uintptr_t)hPcs + (uintptr_t)0x35D240);

        //CreateMTVHostFunc = (long(*)(void*, uint32_t, void*, void*, void*))((uintptr_t)hPcs + (uintptr_t)0x35E4D0);
        //ApplicationViewFilteredCollection_CreateInstanceFunc = (HRESULT(*)(void*, void*, void*, void*, void**))((uintptr_t)hTw + (uintptr_t)0x38CF90);
        //rv = funchook_prepare(funchook, (void**)&TriggerSnapAssistFromApplicationArrangedNotificationFunc, TriggerSnapAssistFromApplicationArrangedNotificationHook);
        //rv = funchook_prepare(funchook, (void**)&CreateMTVHostFunc, CreateMTVHostHook);

        LaunchCortanaApp = (HRESULT(*)(const wchar_t*, uint32_t, bool))((uintptr_t)hTw + (uintptr_t)0x3F93C4);
        GetWindowForView = (HWND(*)(void*))((uintptr_t)hTw + (uintptr_t)0x3D22C);
        OnMessageFunc = (HRESULT(*)(void*, UINT, WPARAM, LPARAM))((uintptr_t)hTw + (uintptr_t)0xAED90);
        ShowSnapAssistFunc = (HRESULT(*)(void*, void*, CRECT*, uint32_t))((uintptr_t)hTw + (uintptr_t)0x38771C);
        ItemSelectedFunc = (HRESULT(*)(void*, void*, CRECT*))((uintptr_t)hTw + (uintptr_t)0x386460);
        rv = funchook_prepare(funchook, (void**)&OnMessageFunc, OnMessageHook);
        if (rv != 0)
        {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }
        rv = funchook_prepare(funchook, (void**)&ShowSnapAssistFunc, ShowSnapAssistHook);
        if (rv != 0)
        {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }
        rv = funchook_prepare(funchook, (void**)&ItemSelectedFunc, ItemSelectedHook);
        if (rv != 0) 
        {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }
        rv = funchook_install(funchook, 0);
        if (rv != 0) 
        {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }

        Sleep(200);
        CreateThread(
            NULL,
            NULL,
            KeysThread,
            NULL,
            NULL,
            NULL
        );
        char szLibPath[_MAX_PATH + 5];
        szLibPath[0] = '"';
        GetModuleFileNameA(
            hModule,
            szLibPath + 1,
            _MAX_PATH
        );
        PathRemoveFileSpecA(szLibPath + 1);
        strcat_s(
            szLibPath,
            "\\WinOverview2.exe\" 1"
        );
        WinExec(
            szLibPath,
            SW_SHOW
        );
        WinExec(
            szLibPath,
            SW_SHOW
        );
    }
    else
    {
        rv = funchook_uninstall(funchook, 0);
        if (rv != 0)
        {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }

        rv = funchook_destroy(funchook);
        if (rv != 0)
        {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }

        FreeLibraryAndExitThread(hModule, 0);
    }

    return 0;
}

BOOL WINAPI DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD     fdwReason,
    _In_ LPVOID    lpvReserved
)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        hModule = hinstDLL;
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}