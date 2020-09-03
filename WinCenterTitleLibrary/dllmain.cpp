#include <iostream>
#include <Windows.h>
#include <funchook.h>

#define ENTIRE_TITLEBAR 10000
#define SET_COLOR
#define TEXT_COLOR RGB(0, 0, 0)
#undef SET_COLOR

funchook_t* funchook = NULL;
HMODULE hModule = NULL;
BYTE* titlebar_color = NULL;
BYTE old_titlebar_color;

static int(*DrawTextWFunc)(
    HDC     hdc,
    LPCWSTR lpchText,
    int     cchText,
    LPRECT  lprc,
    UINT    format
    );

int DrawTextWHook(
    HDC     hdc,
    LPCWSTR lpchText,
    int     cchText,
    LPRECT  lprc,
    UINT    format
)
{
    int ret;

    if (!(format & DT_CALCRECT))
    {
        // fixes title label sometimes overlapping right margin of window
        lprc->right -= 2;
#ifdef SET_COLOR
        SetTextColor(hdc, TEXT_COLOR);
#endif
    }
    ret = DrawTextWFunc(
        hdc,
        lpchText,
        cchText,
        lprc,
        (format & (~DT_LEFT)) | DT_CENTER
    );
    if (format & DT_CALCRECT) lprc->right = ENTIRE_TITLEBAR;
    return ret;
}

__declspec(dllexport) DWORD WINAPI main(
    _In_ LPVOID lpParameter
)
{
    FILE* conout;
    int rv;
   
    if (!funchook)
    {
        funchook = funchook_create();

        DrawTextWFunc = DrawTextW;
        rv = funchook_prepare(funchook, (void**)&DrawTextWFunc, DrawTextWHook);
        if (rv != 0) 
        {
            return rv;
        }

        rv = funchook_install(funchook, 0);
        if (rv != 0) 
        {
            return rv;
        }

        // determine aero.msstyles code path flag location
        if (titlebar_color == NULL)
        {
            HANDLE hudwm = GetModuleHandle(L"uDWM");
            uintptr_t* g_pdmInstance = (uintptr_t*)((uintptr_t)hudwm + (uintptr_t)(0xE6D88));
            titlebar_color = (BYTE*)((uintptr_t)(*g_pdmInstance) + (uintptr_t)0x19);
            old_titlebar_color = *titlebar_color;
        }
        // 1 = white title bars, 0 = colored title bars
        *titlebar_color = 1;
    }
    else
    {
        rv = funchook_uninstall(funchook, 0);
        if (rv != 0)
        {
            return rv;
        }

        rv = funchook_destroy(funchook);
        if (rv != 0)
        {
            return rv;
        }

        *titlebar_color = old_titlebar_color;

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