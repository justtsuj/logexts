#include <tchar.h>
#include <Windows.h>
#include <iostream>
#include <Psapi.h>

#include "..\logexts\logexts.h"

//extern "C" __declspec(dllimport) void logi(HANDLE hCurrentProcess, HANDLE hCurrentThread, ULONG64 dwCurrentPc, ULONG dwProcessor, PCSTR args);
//extern "C" __declspec(dllimport) void logir(HANDLE hCurrentProcess, HANDLE hCurrentThread, ULONG64 dwCurrentPc, ULONG dwProcessor, PCSTR args);
//__declspec(dllimport) void OutputManifest();
//extern "C" __declspec(dllimport) void DetoursTest();
__declspec(dllimport) void __fastcall InjectionCleanup(InjectParameter* injectParameter);

int _tmain(int argc, _TCHAR* argv[])
{
    //STARTUPINFO si;
    //PROCESS_INFORMATION pi;

    //ZeroMemory(&si, sizeof(si));
    //si.cb = sizeof(si);
    //ZeroMemory(&pi, sizeof(pi));
    //_TCHAR cmd[MAX_PATH] = TEXT("D:\\tool\\LoadDll\\Release\\LoadDll.exe C:\\Windows\\SysWOW64\\kernel32.dll");
    //if (!CreateProcess(NULL,   // No module name (use command line)
    //    cmd,        // Command line
    //    NULL,           // Process handle not inheritable
    //    NULL,           // Thread handle not inheritable
    //    FALSE,          // Set handle inheritance to FALSE
    //    CREATE_SUSPENDED,
    //    NULL,           // Use parent's environment block
    //    NULL,           // Use parent's starting directory 
    //    &si,            // Pointer to STARTUPINFO structure
    //    &pi)           // Pointer to PROCESS_INFORMATION structure
    //    )
    //{
    //    printf("CreateProcess failed (%d).\n", GetLastError());
    //    return 1;
    //}
    ////logi test
    //logi(pi.hProcess, pi.hThread, 0, 0, nullptr);
    //ResumeThread(pi.hThread);

    //// Wait until child process exits.
    //WaitForSingleObject(pi.hProcess, INFINITE);

    //// Close process and thread handles. 
    //CloseHandle(pi.hProcess);
    //CloseHandle(pi.hThread);
    InjectionCleanup(nullptr);
    LoadLibrary(TEXT(""));
    return 0;
}