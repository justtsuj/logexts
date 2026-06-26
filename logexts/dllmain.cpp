// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "logexts.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    LPTSTR delim = nullptr;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        OutputDebugString(TEXT("Logexts Process Attach\r\n"));
        LogextsAttach(hModule);
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        LogextsDetach();
        OutputDebugString(TEXT("Logexts Process Detach\r\n"));
        break;
    }
    return true;
}