#include "pch.h"
#include "util.h"
#include "logconf.h"
#include "Manifest/Manifest.h"
#include "FunctionWriter.h"
#include "Logger.h"

extern ApiLogFile::Logger* g_Logger;
extern LogConfig logConfig;

using namespace ApiLogFile;

HookInfo::~HookInfo(){
    index = 0;
    originalAddress = nullptr;
    jmpAddress = nullptr;
    isHook = false;
}

LogHookSet::~LogHookSet(){
    //解除hook
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    for (size_t i = 0; i < size; i++) {
        HookInfo* curHookInfo = hookInfoTable + i;
        if (curHookInfo->originalAddress && curHookInfo->jmpAddress) {
            LONG err = DetourDetach(&curHookInfo->originalAddress, curHookInfo->jmpAddress);
            //如果被hook的模块被释放了，那这样detach肯定会有异常
        }
    }
    DetourTransactionCommit();
    VirtualFree(jmpTable, 0, MEM_RELEASE);
    jmpTable = nullptr;
    delete[] hookInfoTable;
    hookInfoTable = nullptr;
}

STDCALL_HOOK_PROVIDER::STDCALL_HOOK_PROVIDER(/* args */) : logHookSet(){
}

STDCALL_HOOK_PROVIDER::~STDCALL_HOOK_PROVIDER()
{
}

//void LogHook(){
//    // 首先保存hook程序要使用的寄存器
//    OutputDebugString(TEXT("here\r\n"));
//    // 恢复保存的寄存器
//}

extern "C" DWORD_PTR __stdcall _LogHookCallFunction(DWORD_PTR r_ebp, PVOID real_func, PDWORD_PTR p_num_of_param);
DWORD_PTR LogProcessHookCallFunction(DWORD_PTR r_ebp, PVOID real_func, PDWORD_PTR p_num_of_param) {
    return _LogHookCallFunction(r_ebp, real_func, p_num_of_param);
}

bool IsLoggingEnabled(LogModule* logModule) {
    return (logModule->flag & 8) == 0 && (logModule->flag & 2) == 0;
}

DWORD_PTR StdCallLogHookWithPfnOld(DWORD_PTR r_ebp, DWORD_PTR retAddr, DWORD hook_table_index) {
    DWORD_PTR retValue = 0, num_of_param = 0;
    FunctionWriter functionWriter;
    HookInfo* curHookInfo = g_Logger->stdcallProvider->logHookSet.hookInfoTable + hook_table_index;
    ApiManifest::Function* function = g_Logger->manifest.functions[curHookInfo->index];
    LogModule* logModule = g_Logger->moduleList.GetModuleFromAddress((PBYTE)retAddr);

    if (function) {
#ifdef _WIN64
        num_of_param = function->parametersSize / 8;
#else
        num_of_param = function->parametersSize / 4;
#endif // _WIN64
    }
    else {
        num_of_param = 0x2f;
    }
    // 是否记录
    LPCTSTR modName = nullptr;
    DWORD_PTR offset = 0;
    if (!logModule) {
        if (retAddr < 0x60000000) offset = retAddr; //shellcode
        // offset = retAddr;
    }
    else if (IsLoggingEnabled(logModule)) {
        modName = logModule->modName; offset = logModule->GetOffsetFromAddress((PBYTE)retAddr);
    }
    if(offset) functionWriter.WriteEnter(modName, offset, r_ebp, function);

    retValue = LogProcessHookCallFunction(r_ebp, curHookInfo->originalAddress, &num_of_param);
    retValue = g_Logger->stdcallProvider->HandleSpecialAPILeave(function, retValue, retAddr);

    if (offset) functionWriter.WriteLeave(r_ebp, function, retValue);

    return retValue;
}

DWORD_PTR StdCallLogHook(DWORD_PTR r_ebp, DWORD_PTR retAddr, DWORD hook_table_index) {
    /*HookInfo* curHookInfo = g_Logger->stdcallProvider->logHookSet.hookInfoTable + hook_table_index;*/
    // g_Logger->manifest.functions[curHookInfo->index], curHookInfo->originalAddress
    return StdCallLogHookWithPfnOld(r_ebp, retAddr, hook_table_index);
}

extern "C" DWORD_PTR __stdcall LogProcessHook(DWORD_PTR r_ebp, DWORD_PTR retAddr, DWORD index) {
    //LogModule* logModule = g_Logger->moduleList.GetModuleFromAddress((PBYTE)retAddr);
    //bool isLog = !logModule || IsLoggingEnabled(logModule);
    return StdCallLogHook(r_ebp, retAddr, index & 0x0fffffff);
}

extern "C" DWORD_PTR __stdcall _LogHook();
bool STDCALL_HOOK_PROVIDER::SetupHookSet(ApiManifest::Manifest *manifest){
    return true;
}

bool inList() {
    return false;
}

bool STDCALL_HOOK_PROVIDER::BuildHooks(ApiManifest::Manifest& manifest){
    // LogHookSet 初始化
    logHookSet.size = 0;
    for (ApiManifest::Function* function : manifest.functions) {
        //自定义排除的dll
        bool flag = false;
        for (const std::string& s : logConfig.excludeDlls) {
            if (!_stricmp(s.c_str(), function->moduleName.c_str())) {
                flag = true;
                break;
            }
        }
        if (flag) continue;
        // 自定义排除的api
        for (const std::string& s : logConfig.excludeApis) {
            if (!_strnicmp(s.c_str(), function->name.c_str(), s.length())) {
                flag = true;
                break;
            }
        }
        if (flag) continue;
        if (function->GetComInterface() || function->IsDeprecated()) continue;
        ++logHookSet.size;
    }
    logHookSet.jmpTable = reinterpret_cast<Stage*>(VirtualAlloc(0, logHookSet.size * sizeof(Stage), 0x1000u, 0x40u));
    logHookSet.hookInfoTable = new HookInfo[manifest.functions.size()];

    //记录特殊函数
    fLoadLibraryA = manifest.FindFunctionByName("LoadLibraryA");
    fLoadLibraryW = manifest.FindFunctionByName("LoadLibraryW");
    fLoadLibraryExA = manifest.FindFunctionByName("LoadLibraryExA");
    fLoadLibraryExW = manifest.FindFunctionByName("LoadLibraryEXW");

    unsigned int index = 0x10000000;
    Stage* curStage = logHookSet.jmpTable;
    HookInfo* curHookInfo = logHookSet.hookInfoTable;
    for (ApiManifest::Function* function : manifest.functions) {
        //自定义排除的dll
        bool flag = false;
        for (const std::string& s : logConfig.excludeDlls) {
            if (!_stricmp(s.c_str(), function->moduleName.c_str())){
                flag = true;
                break;
            }
        }
        if (flag) continue;
        // 自定义排除的api
        for (const std::string& s : logConfig.excludeApis) {
            if (!_strnicmp(s.c_str(), function->name.c_str(), s.length())){
                flag = true;
                break;
            }
        }
        if (flag) continue;
        if (function->GetComInterface() || function->IsDeprecated()) continue;
#ifdef _WIN64
        curStage->code[0] = 0x90;
        curStage->code[1] = 0x90;
        curStage->code[2] = 0x90;
        curStage->code[3] = 0x90;
        curStage->code[4] = 0x90;
        curStage->code[5] = 0x90;
        curStage->code[6] = 0x90;
        curStage->code[7] = 0x90;
        curStage->code[8] = 0x90;
        curStage->code[9] = 0x90;
        curStage->code[10] = 0x90;
        curStage->code[11] = 0x90;
        curStage->code[12] = 0x41;
        curStage->code[13] = 0xbb;
        *(unsigned int*)(curStage->code + 14) = index;
        curStage->code[18] = 0xff;
        curStage->code[19] = 0x25;
        curStage->code[20] = 0;
        curStage->code[21] = 0;
        curStage->code[22] = 0;
        curStage->code[23] = 0;
        *(DWORD64*)(curStage->code + 24) = (DWORD64)_LogHook;
#else
        curStage->code[0] = 0x90;
        curStage->code[1] = 0x90;
        curStage->code[2] = 0x90;
        curStage->code[3] = 0x90;
        curStage->code[4] = 0x90;
        curStage->code[5] = 0x90;
        curStage->code[6] = 0x68;
        *(unsigned int*)(curStage->code + 7) = index;
        curStage->code[11] = 0xe9;
        *(unsigned int*)(curStage->code + 12) = (unsigned int)_LogHook - (unsigned int)curStage - 16;
#endif // _WIN64
        curHookInfo->index = function->index;
        curHookInfo->originalAddress = nullptr;
        curHookInfo->jmpAddress = curStage;
        curHookInfo->functionName = function->name;
        curHookInfo->moduleName = function->moduleName;
        curStage += 1;
        curHookInfo += 1;
        index += 1;
    }
    return true;
}

HookInfo* STDCALL_HOOK_PROVIDER::GetHookOfStdCallFunctionByName(LPCSTR _functionName) {
    HookInfo* curHookInfo = logHookSet.hookInfoTable;
    for (size_t i = 0; i < logHookSet.size; ++i, ++curHookInfo) {
        if (!curHookInfo->functionName.compare(_functionName)) return curHookInfo;
    }
    return nullptr;
}

DWORD_PTR STDCALL_HOOK_PROVIDER::HandleSpecialAPILeave(ApiManifest::Function* function, DWORD_PTR retValue, DWORD_PTR retAddr) {
    if (function == fLoadLibraryA || function == fLoadLibraryW || function == fLoadLibraryExA || function == fLoadLibraryExW) {
        //DebugOut(TEXT("%s %tx %tx\r\n"), TEXT("second task snapshot"), retAddr, retValue);
        g_Logger->UpdateModuleList();
    }
    return retValue;
}

// HOOK_HASH_TABLE::HOOK_HASH_TABLE(/* args */)
// {
// }

// HOOK_HASH_TABLE::~HOOK_HASH_TABLE()
// {
// }

// bool HOOK_HASH_TABLE::Exists(void *ptr)
// {
//     return true;
// }

// bool HOOK_HASH_TABLE::AddHooks(STDCALL_HOOK_PROVIDER *stdcallProvider)
// {
//     for (size_t i = 0; i < stdcallProvider->size; i++)
//     {
//         if(stdcallProvider->hookInfo[i].originalAddress == nullptr){
//             HMODULE hModule = GetModuleHandleA(stdcallProvider->hookInfo[i].moduleName);
//             if(hModule == nullptr) continue;
//             stdcallProvider[i].hookInfo->originalAddress = GetProcAddress(hModule, stdcallProvider->hookInfo[i].functionName);
//             if(stdcallProvider[i].hookInfo->originalAddress == nullptr) OutputDebugString(TEXT("Ignoring %s because UKGetProcAddress failed"));
//         }
//         if
//     }
//     return true;
// }

DETOURS_HOOK_MANAGER::DETOURS_HOOK_MANAGER()
{
}

DETOURS_HOOK_MANAGER::~DETOURS_HOOK_MANAGER()
{
}

bool DETOURS_HOOK_MANAGER::Initialize()
{
    return true;
}

bool DETOURS_HOOK_MANAGER::AddHookSet(LogHookSet* logHookSet) {
    logHookSetList.push_back(logHookSet);
    return true;
}

bool DETOURS_HOOK_MANAGER::HookNewModules(std::vector<LogModule*>& moduleList) {
    EnterCriticalSection(&g_Logger->CriticalSection);
    // hook module
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourSetIgnoreTooSmall(TRUE);
    for (LogHookSet* logHookSet : logHookSetList) {
        for (size_t i = 0; i < logHookSet->size; i++) {
            HookInfo* curHookInfo = logHookSet->hookInfoTable + i;
            if (curHookInfo->isHook) continue;
            // 获取函数真实地址
            if (curHookInfo->originalAddress == nullptr) {

                HMODULE hModule = GetModuleHandleA(curHookInfo->moduleName.c_str());
                if (hModule == nullptr) {
                    //OutputDebugString(TEXT("Module not found"));
                    continue;
                }
                curHookInfo->originalAddress = GetProcAddress(hModule, curHookInfo->functionName.c_str());
                if (curHookInfo->originalAddress == nullptr) {
                    //OutputDebugString(TEXT("Ignoring %s because UKGetProcAddress failed"));
                    continue;
                }
            }
            if (curHookInfo->originalAddress && curHookInfo->jmpAddress) {
                try
                {
                    if (logConfig.isDebug) DebugOut(TEXT("detours %hs!%hs, %p -> %p\r\n"), curHookInfo->moduleName.c_str(), curHookInfo->functionName.c_str(), curHookInfo->originalAddress, curHookInfo->jmpAddress);
                    LONG err = DetourAttach(&curHookInfo->originalAddress, curHookInfo->jmpAddress);
                    curHookInfo->isHook = true;
#ifdef _DEBUG
                    if (err != NO_ERROR)
                        DebugOut(TEXT("detours %hs, error %ld\r\n"), curHookInfo->functionName.c_str(), err);
#endif // DEBUG
                }
                catch (...)
                {
                    LeaveCriticalSection(&g_Logger->CriticalSection);
                    return false;
                }

            }
        }
    }
    DetourTransactionCommit();
    LeaveCriticalSection(&g_Logger->CriticalSection);
    return true;
}

IAT_HOOK_MANAGER::IAT_HOOK_MANAGER(/* args */)
{
}

IAT_HOOK_MANAGER::~IAT_HOOK_MANAGER()
{
}

bool IAT_HOOK_MANAGER::Initialize()
{
    return true;
}

bool IAT_HOOK_MANAGER::AddHookSet(LogHookSet* logHookSet)
{
    for (size_t i = 0; i < logHookSet->size; i++){
        HookInfo *curHookInfo = logHookSet->hookInfoTable + i;
        if(curHookInfo->originalAddress == nullptr){
            HMODULE hModule = GetModuleHandleA(curHookInfo->moduleName.c_str());
            if(hModule == nullptr){
                OutputDebugString(TEXT("Module not found"));
                continue;
            }
            curHookInfo->originalAddress = GetProcAddress(hModule, curHookInfo->functionName.c_str());
            if(curHookInfo->originalAddress == nullptr){
                //OutputDebugString(TEXT("Ignoring %s because UKGetProcAddress failed"));
                continue;
            }
        }
        if(hookHashTable.find(curHookInfo->originalAddress) == hookHashTable.end())
            hookHashTable.insert(std::pair<void*, void*>(curHookInfo->originalAddress, curHookInfo->jmpAddress));
    }
    return true;
}

bool IAT_HOOK_MANAGER::PatchModule(LogModule &logModule, STDCALL_HOOK_PROVIDER* stdcallProvider){
    LPVOID imageBase = logModule.modBaseAddr;
    PIMAGE_DOS_HEADER dosHeaders = (PIMAGE_DOS_HEADER)imageBase;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((DWORD_PTR)imageBase + dosHeaders->e_lfanew);

    IMAGE_DATA_DIRECTORY iatDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT];
    if (iatDirectory.VirtualAddress == 0 or iatDirectory.Size == 0) return false;
    DWORD oldProtect = 0;
    VirtualProtect((LPVOID)((DWORD_PTR)imageBase + iatDirectory.VirtualAddress), iatDirectory.Size, PAGE_READWRITE, &oldProtect);
    // fail

    IMAGE_DATA_DIRECTORY importsDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importsDirectory.VirtualAddress == 0 or importsDirectory.Size == 0) return false;
    PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(importsDirectory.VirtualAddress + (DWORD_PTR)imageBase);
    LPCSTR libraryName = nullptr;
    
    while (importDescriptor->Name != NULL)
    {
        libraryName = (LPCSTR)importDescriptor->Name + (DWORD_PTR)imageBase;
        OutputDebugString(TEXT("hook "));
        OutputDebugStringA(libraryName);
        OutputDebugString(TEXT("\r\n"));
        PIMAGE_THUNK_DATA originalFirstThunk = (PIMAGE_THUNK_DATA)((DWORD_PTR)imageBase + importDescriptor->OriginalFirstThunk);
        PIMAGE_THUNK_DATA firstThunk = (PIMAGE_THUNK_DATA)((DWORD_PTR)imageBase + importDescriptor->FirstThunk);

        while (originalFirstThunk->u1.AddressOfData != NULL)
        {
            if (hookHashTable.find((void*)firstThunk->u1.Function) != hookHashTable.end())
                // 考虑使用原子操作
                firstThunk->u1.Function = (DWORD_PTR)hookHashTable[(void*)firstThunk->u1.Function];
            else {
                PIMAGE_IMPORT_BY_NAME functionName = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)imageBase + originalFirstThunk->u1.AddressOfData);
                HookInfo *hookInfo = stdcallProvider->GetHookOfStdCallFunctionByName(functionName->Name);
                if (hookInfo) {
                    //printf("%p %p\r\n", firstThunk->u1.Function, hookInfo->jmpAddress);
                    hookHashTable.insert(std::pair<void*, void*>((void*)firstThunk->u1.Function, hookInfo->jmpAddress));
                    firstThunk->u1.Function = (DWORD_PTR)hookInfo->jmpAddress;
                }
            }
            ++originalFirstThunk;
            ++firstThunk;
        }
        importDescriptor++;
    }
    //恢复iat内存属性
    return true;
}

//bool IAT_HOOK_MANAGER::HookOneModule(LOG_MODULE &logModule){
//    return PatchModule(logModule);
//}
//
//bool IAT_HOOK_MANAGER::HookNewModules(std::vector<LOG_MODULE*> &moduleList){
//    for(LOG_MODULE *logModule : moduleList){
//        HookOneModule(*logModule);
//    }
//    return true;
//}

Logger::Logger(/* args */)
{
}

Logger::~Logger()
{
    DeleteCriticalSection(&CriticalSection);
    delete hookManager;
    hookManager = nullptr;
    delete stdcallProvider;
    stdcallProvider = nullptr;
}

bool Logger::Initialize(){
    InitializeCriticalSectionAndSpinCount(&CriticalSection, 0x80000000);
    stdcallProvider = new STDCALL_HOOK_PROVIDER();
    //获取manfest文件路径，当前设置在当前模块路径下
    TCHAR logextManifestFilepath[MAX_PATH];
    _tcscpy_s(logextManifestFilepath, MAX_PATH, logConfig.curDirp);
    //使用原始的二进制文件
    _tcscat_s(logextManifestFilepath, MAX_PATH, TEXT("LogManifest.lgm"));
    std::ifstream reader(logextManifestFilepath, std::ifstream::binary);
    if (!reader.good()) {
        OutputDebugString(TEXT("Error opening manifest file\r\n"));
        return false;
    }
    manifest.Read(reader);
    //使用json文件，太慢了
    //_tcscat_s(logextManifestFilepath, MAX_PATH, TEXT("manifest.json"));
    //std::ifstream f(logextManifestFilepath);
    //nlohmann::json data = nlohmann::json::parse(f);
    //f.close();
    //data.get_to(manifest);
    //manifest.RebuildLinks();

    stdcallProvider->BuildHooks(manifest);
    return true;
}

bool Logger::UpdateModuleList(){
    //记录下系统模块的基址
    logConfig.ntdll = GetModuleHandle(TEXT("ntdll"));
    logConfig.kernel32 = GetModuleHandle(TEXT("kernel32"));
    logConfig.kernelBase = GetModuleHandle(TEXT("kernelbase"));

    std::vector<LogModule*> newModuleList, reloadModuleList;
    moduleList.TakeSnapshot(newModuleList, reloadModuleList);
    hookManager->HookNewModules(newModuleList);
    //hookManager->HookNewModules(oldModuleList);
    return true;
}