#include <valinet/ini/ini.h>
#include <valinet/pdb/pdb.h>
#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>
#include <funchook.h>
#pragma comment(lib, "Psapi.lib") // required by funchook
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#define DUMMY_CLASS_NAME L"WinOverview2_dummy"
#define WPARAM_CORTANA 33
#define WM_SETFOREGROUND WM_USER + 1
#define WM_OVERVIEW WM_USER + 2
#define NUMBER_OF_HOOKED_FUNCTIONS 5
#define DLL_NAME "twinui.dll"

#define DEFAULT_ADDR0 0
#define DEFAULT_ADDR1 0
#define DEFAULT_ADDR2 0
#define DEFAULT_ADDR3 0
#define DEFAULT_ADDR4 0

#define ADDR_STR0               "LaunchCortanaApp"
#define ADDR_STR1               "ApplicationViewHelpers::GetWindowForView"
#define ADDR_STR2               "CImmersiveHotkeyNotification::OnMessage"
#define ADDR_STR3               "CSnapAssistControllerBase::_ShowSnapAssist"
#define ADDR_STR4               "CSnapAssistControllerBase::ItemSelected"

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
                    MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY),
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
            /*
            MessageBox(
                NULL,
                L"To enable this feature, snap any window to the side of the screen.",
                L"Windows Explorer",
                0
            );
            */
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
    /*SendMessage(
        messageWindow,
        WM_SETFOREGROUND,
        (WPARAM)hWnd,
        NULL
    );*/
    SetForegroundWindow(hWnd);
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
    
    /*FILE* conout;
    AllocConsole();
    freopen_s(&conout, "CONOUT$", "w", stdout);*/
    

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
        
        SIZE_T dwRet = 0;
        BOOL bErr = FALSE;
        char* symbolNames[NUMBER_OF_HOOKED_FUNCTIONS] = {
            ADDR_STR0,
            ADDR_STR1,
            ADDR_STR2,
            ADDR_STR3,
            ADDR_STR4
        };
        DWORD addresses[NUMBER_OF_HOOKED_FUNCTIONS];
        ZeroMemory(addresses, NUMBER_OF_HOOKED_FUNCTIONS * sizeof(DWORD));
        BOOL hooked[3];
        ZeroMemory(hooked, 3 * sizeof(BOOL));
        char szLibPath[_MAX_PATH];
        TCHAR wszLibPath[_MAX_PATH];
        ZeroMemory(szLibPath, (_MAX_PATH) * sizeof(char));
        ZeroMemory(wszLibPath, (_MAX_PATH) * sizeof(TCHAR));
        GetModuleFileNameA(
            hModule,
            szLibPath,
            _MAX_PATH
        );
        PathRemoveFileSpecA(szLibPath);
        strcat_s(
            szLibPath,
            _MAX_PATH,
            "\\settings.ini"
        );
        mbstowcs_s(
            &dwRet, 
            wszLibPath, 
            _MAX_PATH, 
            szLibPath, 
            _MAX_PATH
        );

        addresses[0] = VnGetUInt(
            TEXT("Addresses"), 
            TEXT(ADDR_STR0),
            DEFAULT_ADDR0,
            wszLibPath
        );
        addresses[1] = VnGetUInt(
            TEXT("Addresses"), 
            TEXT(ADDR_STR1), 
            DEFAULT_ADDR1,
            wszLibPath
        );
        addresses[2] = VnGetUInt(
            TEXT("Addresses"), 
            TEXT(ADDR_STR2), 
            DEFAULT_ADDR2,
            wszLibPath
        );
        addresses[3] = VnGetUInt(
            TEXT("Addresses"), 
            TEXT(ADDR_STR3), 
            DEFAULT_ADDR3,
            wszLibPath
        );
        addresses[4] = VnGetUInt(
            TEXT("Addresses"), 
            TEXT(ADDR_STR4), 
            DEFAULT_ADDR4,
            wszLibPath
        );
        do {
            LaunchCortanaApp = (HRESULT(*)(const wchar_t*, uint32_t, bool))((uintptr_t)hTw +
                (uintptr_t)addresses[0]);
            GetWindowForView = (HWND(*)(void*))((uintptr_t)hTw +
                (uintptr_t)addresses[1]);
            OnMessageFunc = (HRESULT(*)(void*, UINT, WPARAM, LPARAM))((uintptr_t)hTw +
                (uintptr_t)addresses[2]);
            ShowSnapAssistFunc = (HRESULT(*)(void*, void*, CRECT*, uint32_t))((uintptr_t)hTw +
                (uintptr_t)addresses[3]);
            ItemSelectedFunc = (HRESULT(*)(void*, void*, CRECT*))((uintptr_t)hTw +
                (uintptr_t)addresses[4]);
            if (addresses[0] == 0 ||
                addresses[1] == 0 ||
                addresses[2] == 0 ||
                addresses[3] == 0 ||
                addresses[4] == 0
            )
            {
                bErr = TRUE;
            }
            if (!bErr)
            {
                if (!hooked[0])
                {
                    rv = funchook_prepare(
                        funchook, 
                        (void**)&OnMessageFunc,
                        OnMessageHook
                    );
                    if (rv != 0)
                    {
                        if (!bErr)
                        {
                            bErr = TRUE;
                        }
                        else
                        {
                            FreeLibraryAndExitThread(hModule, rv);
                            return rv;
                        }
                    }
                    else
                    {
                        hooked[0] = TRUE;
                    }
                }
                if (!hooked[1])
                {
                    rv = funchook_prepare(
                        funchook, 
                        (void**)&ShowSnapAssistFunc, 
                        ShowSnapAssistHook
                    );
                    if (rv != 0)
                    {
                        if (!bErr)
                        {
                            bErr = TRUE;
                        }
                        else
                        {
                            FreeLibraryAndExitThread(hModule, rv);
                            return rv;
                        }
                    }
                    else
                    {
                        hooked[1] = TRUE;
                    }
                }
                if (!hooked[2])
                {
                    rv = funchook_prepare(
                        funchook, 
                        (void**)&ItemSelectedFunc,
                        ItemSelectedHook
                    );
                    if (rv != 0)
                    {
                        if (!bErr)
                        {
                            bErr = TRUE;
                        }
                        else
                        {
                            FreeLibraryAndExitThread(hModule, rv);
                            return rv;
                        }
                    }
                    else
                    {
                        hooked[2] = TRUE;
                    }
                }
                if (hooked[0] && hooked[1] && hooked[2])
                {
                    bErr = FALSE;
                }
            }
            if (bErr)
            {
                GetModuleFileNameA(
                    hModule,
                    szLibPath,
                    _MAX_PATH
                );
                PathRemoveFileSpecA(szLibPath);
                strcat_s(
                    szLibPath,
                    _MAX_PATH,
                    "\\"
                );
                if (VnDownloadSymbols(
                    hModule, 
                    DLL_NAME, 
                    szLibPath, 
                    _MAX_PATH
                ))
                {
                    FreeLibraryAndExitThread(hModule, 0x101);
                    return 0x101;
                }
                if (VnGetSymbols(
                    szLibPath, 
                    addresses,
                    symbolNames,
                    NUMBER_OF_HOOKED_FUNCTIONS
                ))
                {
                    FreeLibraryAndExitThread(hModule, 0x100);
                    return 0x100;
                }
                VnWriteUInt(
                    TEXT("Addresses"),
                    TEXT(ADDR_STR0),
                    addresses[0],
                    wszLibPath
                );
                VnWriteUInt(
                    TEXT("Addresses"),
                    TEXT(ADDR_STR1),
                    addresses[1],
                    wszLibPath
                );
                VnWriteUInt(
                    TEXT("Addresses"),
                    TEXT(ADDR_STR2),
                    addresses[2],
                    wszLibPath
                );
                VnWriteUInt(
                    TEXT("Addresses"),
                    TEXT(ADDR_STR3),
                    addresses[3],
                    wszLibPath
                );
                VnWriteUInt(
                    TEXT("Addresses"),
                    TEXT(ADDR_STR4),
                    addresses[4],
                    wszLibPath
                );
            }
        } while (bErr);
        rv = funchook_install(funchook, 0);
        if (rv != 0) 
        {
            FreeLibraryAndExitThread(hModule, rv);
            return rv;
        }

        Sleep(1000);
        CreateThread(
            NULL,
            NULL,
            KeysThread,
            NULL,
            NULL,
            NULL
        );
        ZeroMemory(szLibPath, (_MAX_PATH) * sizeof(char));
        szLibPath[0] = '"';
        GetModuleFileNameA(
            hModule,
            szLibPath + 1,
            _MAX_PATH
        );
        PathRemoveFileSpecA(szLibPath + 1);
        strcat_s(
            szLibPath,
            _MAX_PATH,
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