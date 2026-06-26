#include <nlohmann/json.hpp>

#include "pch.h"
#include "util.h"
#include "logconf.h"
#include "Manifest/Manifest.h"
#include "Logger.h"
#include "logexts.h"

#define KDEXT_64BIT
#include <wdbgexts.h>

// windbg plugin struct
EXT_API_VERSION         ApiVersion = { 1, 0, EXT_API_VERSION_NUMBER64, 0 };
WINDBG_EXTENSION_APIS   ExtensionApis;
ULONG SavedMajorVersion;
ULONG SavedMinorVersion;
// 自身使用的全局变量
DebuggeeInfo* g_debuggeeInfo = nullptr;
LogConfig logConfig;
ApiLogFile::Logger *g_Logger = nullptr;


VOID WinDbgExtensionDllInit(PWINDBG_EXTENSION_APIS lpExtensionApis, USHORT MajorVersion, USHORT MinorVersion)
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

LPEXT_API_VERSION ExtensionApiVersion(VOID)
{
    //
    // ExtensionApiVersion should return EXT_API_VERSION_NUMBER64 in order for APIs
    // to recognize 64 bit addresses.  KDEXT_64BIT also has to be defined before including
    // wdbgexts.h to get 64 bit headers for WINDBG_EXTENSION_APIS
    //
    return &ApiVersion;
}

//
// Routine called by debugger after load
//
VOID CheckVersion(VOID)
{
    return;
}


//SharedBlock::SharedBlock(/* args */)
//{
//}
//
//SharedBlock::~SharedBlock()
//{
//}
//
//bool CreateSharedSection(){
//    return true;
//}

typedef void (__fastcall *InjectionCleanupPROC)(InjectParameter* injectParameter);
void __fastcall InjectionCleanup(InjectParameter* injectParameter) {
    g_Logger = new ApiLogFile::Logger();
    if(!g_Logger->Initialize()) return;
    g_Logger->hookManager = new ApiLogFile::DETOURS_HOOK_MANAGER();
    g_Logger->hookManager->Initialize();
    g_Logger->hookManager->AddHookSet(&g_Logger->stdcallProvider->logHookSet);
    //DebugOut(TEXT("%s\r\n"), TEXT("first task snapshot"));
    g_Logger->UpdateModuleList();
}

void __fastcall LogHookHijackInjector(InjectParameter* injectParameter){
    injectParameter->hModule = injectParameter->loadLibraryWPROC(injectParameter->logextMoudleName);
    if (injectParameter->hModule) {
        InjectionCleanupPROC pInjectionCleanup = reinterpret_cast<InjectionCleanupPROC>(injectParameter->getProcAddressPROC(injectParameter->hModule, injectParameter->functionName));
        pInjectionCleanup(injectParameter);
    }
    while (true) __debugbreak();
}

// void UseLess(){}

bool DebuggeeInfo::InitInjectionData(InjectParameter *injectParameter){
    if (!GetModuleFileName(logConfig.thisDll, injectParameter->logextMoudleName, MAX_PATH)) {
        Error(TEXT("GetModuleFileName"));
        return false;
    }
    logirInfo.injectDllSharedMemCode = VirtualAllocEx(hProcess, NULL, logConfig.g_SysInfo.dwPageSize, 0x1000u, 0x40u);
    if (logirInfo.injectDllSharedMemCode == nullptr) return false;
    // 获取函数的地址和大小
    WriteProcessMemory(hProcess, logirInfo.injectDllSharedMemCode, LogHookHijackInjector, logConfig.g_SysInfo.dwPageSize, NULL);
    HMODULE kernel32 = GetModuleHandle(TEXT("kernel32.dll"));
    if (kernel32 == NULL) {
        Error(TEXT("GetModuleHandle"));
        return false;
    }
    injectParameter->loadLibraryWPROC = reinterpret_cast<LoadLibraryWPROC>(GetProcAddress(kernel32, "LoadLibraryW"));
    injectParameter->getProcAddressPROC = reinterpret_cast<GetProcAddressPROC>(GetProcAddress(kernel32, "GetProcAddress"));
    if (injectParameter->loadLibraryWPROC == nullptr || injectParameter->getProcAddressPROC == nullptr) {
        Error(TEXT("GetProcAddress"));
        return false;
    }
    return true;
}

bool DebuggeeInfo::InjectThisDll(){
    InjectParameter injectParameter;
    if(!InitInjectionData(&injectParameter)) return false;
    LPVOID injectDllSharedMemInfo = VirtualAllocEx(hProcess, 0, sizeof(InjectParameter), 0x1000u, 0x40u);
    if (injectDllSharedMemInfo == nullptr) {
        Error(TEXT("VirtualAllocEx"));
        return false;
    }
    CONTEXT context;
    context.ContextFlags = CONTEXT_ALL;
    ExtensionApis.lpGetThreadContextRoutine((ULONG)hThread, &context, sizeof(CONTEXT));
    memcpy(&logirInfo.context, &context, sizeof(CONTEXT));
#ifdef _WIN64
    DWORD64 curRip = context.Rip;
    context.Rip = reinterpret_cast<DWORD64>(logirInfo.injectDllSharedMemCode);
    context.Rcx = reinterpret_cast<DWORD64>(injectDllSharedMemInfo);
    ExtensionApis.lpSetThreadContextRoutine((ULONG)hThread, &context, sizeof(CONTEXT));
    ExtensionApis.lpOutputRoutine("LogExts hijacking current thread @ PC=0x%p with:\r\n", curRip);
    ExtensionApis.lpOutputRoutine("    InjectDll  @ 0x%p\r\n", logirInfo.injectDllSharedMemCode);
    ExtensionApis.lpOutputRoutine("    Data       @ 0x%p\r\n", injectDllSharedMemInfo);
#else
    DWORD curEip = context.Eip;
    context.Eip = reinterpret_cast<DWORD>(logirInfo.injectDllSharedMemCode);
    context.Ecx = reinterpret_cast<DWORD>(injectDllSharedMemInfo);
    ExtensionApis.lpSetThreadContextRoutine((ULONG)hThread, &context, sizeof(CONTEXT));
    ExtensionApis.lpOutputRoutine("LogExts hijacking current thread @ PC=0x%p with:\r\n", curEip);
    ExtensionApis.lpOutputRoutine("    InjectDll  @ 0x%p\r\n", logirInfo.injectDllSharedMemCode);
    ExtensionApis.lpOutputRoutine("    Data       @ 0x%p\r\n", injectDllSharedMemInfo);
#endif
    WriteProcessMemory(hProcess, injectDllSharedMemInfo, &injectParameter, sizeof(InjectParameter), NULL);
    return true;
}

bool DebuggeeInfo::InjectLogexts() {
    // 判断注入状态
    if(injectMode){
        LPCTSTR message = nullptr;
        if(injectMode == 1) message = TEXT("Logexts is already in the process of injecting (need to 'g' until logi/logis occur)");
        else if(injectMode == 2) message = TEXT("Logexts was already injected in that process");
        else message = TEXT("Logexts is in an unknown injection state?!");
        OutputDebugString(message);
        return false;
    }
    injectMode = 1;
    return InjectThisDll();
}

bool LogExtsCmdInit(HANDLE hCurrentProcess, HANDLE hCurrentThread, PDebuggeeInfo *p_debuggeeInfo) {
    // 检查进程状态
    // 正常情况这个流程是，被调试进程中断进调试器，这个handle肯定是有效的
    // 如果被调试进程不是中断的状态，后续一系列进程操作都可能失败
    // 进程必须是被中断的状态
    PDebuggeeInfo cur_debugeeInfo = g_debuggeeInfo;
    while (cur_debugeeInfo) {
        if (cur_debugeeInfo->hProcess == hCurrentProcess) break;
        cur_debugeeInfo = cur_debugeeInfo->next;
    }
    if (!cur_debugeeInfo) {
        cur_debugeeInfo = new DebuggeeInfo;
        if (cur_debugeeInfo == nullptr) {
            OutputDebugString(L"Failure to allocate memory in debugger!");
            return false;
        }
        if(!GetModuleBaseName(hCurrentProcess, NULL, cur_debugeeInfo->mainMoudleName, MAX_PATH))
            Error(TEXT("GetModuleBaseName"));
        //cur_debugeeInfo->sharedBlock = new SharedBlock();
        cur_debugeeInfo->hProcess = hCurrentProcess;
        cur_debugeeInfo->hThread = hCurrentThread;
        cur_debugeeInfo->next = g_debuggeeInfo;
        g_debuggeeInfo = cur_debugeeInfo;
        ExtensionApis.lpOutputRoutine("Windows API Logging Extensions , new process: %s", cur_debugeeInfo->mainMoudleName);
    }
    *p_debuggeeInfo = cur_debugeeInfo;
    // if(cur_link->sharedBlock || CreateSharedSection()){}
    return true;
}

DECLARE_API(logi) {
    PDebuggeeInfo debuggeeInfo = nullptr;
    if (!LogExtsCmdInit(hCurrentProcess, hCurrentThread, &debuggeeInfo)) {
        OutputDebugString(TEXT("init failed\n"));
        return;
    }
    debuggeeInfo->InjectLogexts();
}

int DebuggeeInfo::ResumeFromInjection(HANDLE hCurrentThread){
    if(hThread != hCurrentThread){
        OutputDebugString(TEXT("You are not on the same thread that logi occured on"));
        return 5;
    }
    CONTEXT context;
    context.ContextFlags = CONTEXT_ALL;
    ExtensionApis.lpGetThreadContextRoutine((ULONG)hThread, &context, sizeof(CONTEXT));
#ifdef _WIN64
    if(context.Rip == (DWORD64)logirInfo.injectDllSharedMemCode){
#else
    if (context.Eip == (DWORD)logirInfo.injectDllSharedMemCode) {
#endif
        OutputDebugString(TEXT("This thread is still at InjectDll @ %p, need to 'g' (should break later on in that function)"));
        return 5;
    }
    //if(context.Eip < (DWORD)logirInfo->injectDllSharedMemCode || context.Eip > (DWORD)logirInfo->injectDllSharedMemCode + g_SysInfo.dwPageSize){
    //    OutputDebugString(TEXT("You are at %p, but should be at the breakpoint in InjectDll (%p-%p)"));
    //    return 5;
    //}
    ExtensionApis.lpSetThreadContextRoutine((ULONG)hThread, &logirInfo.context, sizeof(CONTEXT));
    return 0;
}

DECLARE_API(logir){
    PDebuggeeInfo debuggeeInfo = nullptr;
    if (!LogExtsCmdInit(hCurrentProcess, hCurrentThread, &debuggeeInfo)) {
        OutputDebugString(TEXT("init failed\n"));
        return;
    }
    if(debuggeeInfo->injectMode == 1){
        debuggeeInfo->ResumeFromInjection(hCurrentThread);
    }
    else if (debuggeeInfo->injectMode == 2){
        OutputDebugString(TEXT("Logexts!logir already ran"));
    }
    else if(debuggeeInfo->injectMode == 0){
        OutputDebugString(TEXT("Logexts!logi has not been run on this process"));
    }
    else{
        OutputDebugString(TEXT("Logexts is in an unknown injection state?!"));
    }
    
}

DECLARE_API(logd) {
    logConfig.isDebug = !logConfig.isDebug;
}

bool LogextsAttach(HMODULE hModule) {
    logConfig.thisDll = hModule;
    GetSystemInfo(&logConfig.g_SysInfo);
    //初始化模块所在路径
    if (GetModuleFileName(logConfig.thisDll, logConfig.curDirp, MAX_PATH) == 0) {
        Error(TEXT("GetModuleFileName failed"));
        return false;
    }
    LPTSTR delim = _tcsrchr(logConfig.curDirp, TEXT('\\'));
    if (delim == nullptr) {
        OutputDebugString(TEXT("file directory failed"));
        return false;
    }
    *(delim + 1) = 0;
    TCHAR logextConifgFilepath[MAX_PATH];
    _tcscpy_s(logextConifgFilepath, MAX_PATH, logConfig.curDirp);
    _tcscat_s(logextConifgFilepath, MAX_PATH, TEXT("Logexts.json"));
    logConfig.ReadFromJSON(logextConifgFilepath);
    //init SRWLock
    InitializeSRWLock(&logConfig.g_srwLock);
    return true;
}

void LogextsDetach() {
    delete g_Logger;
    g_Logger = nullptr;

    PDebuggeeInfo tmp_debugeeInfo = nullptr;
    while (g_debuggeeInfo) {
        tmp_debugeeInfo = g_debuggeeInfo->next;
        delete g_debuggeeInfo;
        g_debuggeeInfo = tmp_debugeeInfo;
    }
}