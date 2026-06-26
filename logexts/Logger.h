#pragma once

#include "ModuleList.h"

namespace ApiLogFile{
    struct Stage
    {
#ifdef _WIN64
        uint8_t code[32];
#else
        uint8_t code[16];
#endif // _WIN64


    };

    class HookInfo
    {
    public:
        unsigned int index = 0;
        PVOID originalAddress = nullptr;
        PVOID jmpAddress = nullptr;
        std::string moduleName;
        std::string functionName;
        bool isHook = false;

        ~HookInfo();
    };

    class LogHookSet {
    public:
        size_t size;
        Stage* jmpTable;
        HookInfo* hookInfoTable;

        ~LogHookSet();
    };

    class STDCALL_HOOK_PROVIDER
    {
    public:
        LogHookSet logHookSet;  //在hook manager中存有指针
        ApiManifest::Function *fLoadLibraryA = nullptr;
        ApiManifest::Function *fLoadLibraryW = nullptr;
        ApiManifest::Function *fLoadLibraryExA = nullptr;
        ApiManifest::Function *fLoadLibraryExW = nullptr;

        STDCALL_HOOK_PROVIDER(/* args */);
        ~STDCALL_HOOK_PROVIDER();
        bool SetupHookSet(ApiManifest::Manifest *manifest);
        bool BuildHooks(ApiManifest::Manifest& manifest);
        HookInfo* GetHookOfStdCallFunctionByName(LPCSTR _functionName);
        DWORD_PTR HandleSpecialAPILeave(ApiManifest::Function* function, DWORD_PTR retValue, DWORD_PTR retAddr);
    };

    // class HOOK_HASH_TABLE
    // {
    // private:
    //     /* data */
    // public:
    //     HOOK_HASH_TABLE(/* args */);
    //     ~HOOK_HASH_TABLE();
    //     bool AddHooks(STDCALL_HOOK_PROVIDER *stdcallProvider);
    //     bool Exists(void *ptr);
    // };

    class IAT_HOOK_MANAGER
    {
    private:
        /* data */
    public:
        // HOOK_HASH_TABLE hookHashTable;
        std::map<void*, void*> hookHashTable;
        IAT_HOOK_MANAGER(/* args */);
        ~IAT_HOOK_MANAGER();
        bool Initialize();
        bool AddHookSet(LogHookSet* logHookSet);
        bool PatchModule(LogModule &logModule, STDCALL_HOOK_PROVIDER* stdcallProvider);
        //bool HookOneModule(LOG_MODULE &logModule);
        //bool HookNewModules(std::vector<LOG_MODULE*> &moduleList);
    };

    class DETOURS_HOOK_MANAGER
    {
    public:
        std::vector<LogHookSet*> logHookSetList;

        DETOURS_HOOK_MANAGER();
        ~DETOURS_HOOK_MANAGER();

        bool Initialize();
        bool AddHookSet(LogHookSet* logHookSet);
        bool HookNewModules(std::vector<LogModule*> &moduleList);
    };

    class Logger
    {
    public:
        ApiManifest::Manifest manifest;
        CRITICAL_SECTION CriticalSection;
        STDCALL_HOOK_PROVIDER *stdcallProvider = nullptr;
        DETOURS_HOOK_MANAGER *hookManager = nullptr;
        ModuleList moduleList;

        Logger(/* args */);
        ~Logger();
        bool Initialize();
        bool UpdateModuleList();
    };
}