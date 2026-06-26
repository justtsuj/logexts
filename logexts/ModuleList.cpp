#include "pch.h"
#include "util.h"
#include "logconf.h"
#include "ModuleList.h"

extern LogConfig logConfig;

using namespace ApiLogFile;

LogModule::LogModule() : modBaseAddr(0), modImageSize(0), flag(0) {
}

LogModule::~LogModule()
{
}


LogModule::LogModule(PBYTE _modBaseAddr, DWORD _modBaseSize, LPCTSTR _path) :modBaseAddr(_modBaseAddr), modImageSize(_modBaseSize),
    flag(0){
    StringCchCopy(path, MAX_PATH, _path);
    modName = _tcsrchr(path, TEXT('\\')) + 1;
}

DWORD LogModule::GetOffsetFromAddress(PBYTE addr) {
    return addr - modBaseAddr;
}

ModuleList::ModuleList(/* args */)
{

}

ModuleList::~ModuleList()
{
    for (LogModule* logMoudle : allModuleList) {
        delete logMoudle;
    }
}

bool IsSystemFile(LPCTSTR path) {
    return _tcsstr(path, TEXT("\\WINDOWS\\")) || _tcsstr(path, TEXT("\\Windows\\"));
}

LogModule* ModuleList::CheckAddAModule(MODULEENTRY32& mi) {
    for (LogModule* modulee : allModuleList) {
        if (modulee->modBaseAddr == mi.modBaseAddr && modulee->modImageSize == mi.modBaseSize && !_tcscmp(modulee->path, mi.szExePath))
            return modulee;
    }
    LogModule* modulee = new LogModule(mi.modBaseAddr, mi.modBaseSize, mi.szExePath);
    if (IsSystemFile(mi.szExePath)) modulee->flag &= 2;
    if (modulee->modBaseAddr == (PBYTE)logConfig.thisDll ||
        modulee->modBaseAddr == (PBYTE)logConfig.ntdll ||
        modulee->modBaseAddr == (PBYTE)logConfig.kernel32 ||
        modulee->modBaseAddr == (PBYTE)logConfig.kernelBase)
        modulee->flag &= 8; //排除一些模块
    return modulee;
}

// 返回迭代器还是类型
LogModule* ModuleList::FindModule(MODULEENTRY32& mi) {
    for (LogModule* modulee : allModuleList) {
        if (modulee->modBaseAddr == mi.modBaseAddr && modulee->modImageSize == mi.modBaseSize && !_tcscmp(modulee->path, mi.szExePath))
            return modulee;
    }
    return nullptr;
}

LogModule* ModuleList::FindModule(PLDR_DATA_TABLE_ENTRY pEntry) {
    for (LogModule* modulee : allModuleList) {
        if (modulee->modBaseAddr == pEntry->DllBase && modulee->modImageSize == pEntry->SizeOfImage && !wcscmp(modulee->path, pEntry->FullDllName.Buffer))
            return modulee;
    }
    return nullptr;
}

bool ModuleList::TakeSnapshot(std::vector<LogModule*>& newModuleList, std::vector<LogModule*>& reloadModuleList) {
    AcquireSRWLockExclusive(&logConfig.g_srwLock);
    curModuleList.clear();
    //遍历当前的模块
    //HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    //if (INVALID_HANDLE_VALUE == hSnapshot) return false;
    //MODULEENTRY32 mi;
    //mi.dwSize = sizeof(MODULEENTRY32); //第一次使用必须初始化成员
    //BOOL bRet = Module32First(hSnapshot, &mi);
    //while (bRet)
    //{
    //    //OutputDebugString(mi.szModule);
    //    //OutputDebugString(TEXT("\r\n"));
    //    LogModule* logModule = FindModule(mi);
    //    if (logModule) {
    //        if (logModule->flag & 0x10) {
    //            logModule->flag &= 0xFFFFFFEF;
    //            reloadModuleList.push_back(logModule);
    //        }
    //    }
    //    else {
    //        logModule = new LogModule(mi.modBaseAddr, mi.modBaseSize, mi.szExePath);
    //        if (IsSystemFile(mi.szExePath)) logModule->flag |= 2;
    //        if (logModule->modBaseAddr == (PBYTE)logConfig.thisDll ||
    //            logModule->modBaseAddr == (PBYTE)logConfig.ntdll ||
    //            logModule->modBaseAddr == (PBYTE)logConfig.kernel32 ||
    //            logModule->modBaseAddr == (PBYTE)logConfig.kernelBase)
    //            logModule->flag |= 8; //排除一些模块
    //        newModuleList.push_back(logModule);
    //        allModuleList.push_back(logModule);
    //    }
    //    curModuleList.push_back(logModule);
    //    bRet = Module32Next(hSnapshot, &mi);
    //}
    //CloseHandle(hSnapshot);
    //遍历当前的模块，不使用API
#ifdef _M_IX86
    PPEB pPeb = (PPEB)__readfsdword(0x30);
#else
    PPEB pPeb = (PPEB)__readgsqword(0x60);
#endif
    // 获取PEB_LDR_DATA
    PPEB_LDR_DATA pLdr = pPeb->Ldr;
    // 获取模块列表头
    PLIST_ENTRY pModuleListHead = &pLdr->InMemoryOrderModuleList;
    PLIST_ENTRY pModuleListEntry = pModuleListHead->Flink;
    // 遍历模块列表
    while (pModuleListEntry != pModuleListHead) {
        PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pModuleListEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        //DebugOut(TEXT("%s\r\n"), pEntry->FullDllName.Buffer);
        LogModule* logModule = FindModule(pEntry);
        if (logModule) {
            if (logModule->flag & 0x10) {
                logModule->flag &= 0xFFFFFFEF;
                reloadModuleList.push_back(logModule);
            }
        }
        else {
            logModule = new LogModule((PBYTE)pEntry->DllBase, pEntry->SizeOfImage, pEntry->FullDllName.Buffer);
            //标记模块信息
            //系统文件
            if (IsSystemFile(pEntry->FullDllName.Buffer)) logModule->flag |= 2;
            //程序内置的从他们出发的调用不记录
            if (logModule->modBaseAddr == (PBYTE)logConfig.thisDll ||
                logModule->modBaseAddr == (PBYTE)logConfig.ntdll ||
                logModule->modBaseAddr == (PBYTE)logConfig.kernel32 ||
                logModule->modBaseAddr == (PBYTE)logConfig.kernelBase)
                logModule->flag |= 8;
            //自定义的模块从他们出发的调用不记录
            newModuleList.push_back(logModule);
            allModuleList.push_back(logModule);
        }
        curModuleList.push_back(logModule);
        pModuleListEntry = pModuleListEntry->Flink;
    }
    ReleaseSRWLockExclusive(&logConfig.g_srwLock);
    return true;
}

LogModule* ModuleList::GetModuleFromAddress(PBYTE addr) {
    AcquireSRWLockShared(&logConfig.g_srwLock);
    // printf("read lock\r\n");
	for (LogModule* logModule : curModuleList) {
        if (logModule->modBaseAddr <= addr && addr < logModule->modBaseAddr + logModule->modImageSize) {
            // printf("read unlock\r\n");
            ReleaseSRWLockShared(&logConfig.g_srwLock);
            return logModule;
        }
	}
    // printf("read unlock\r\n");
    ReleaseSRWLockShared(&logConfig.g_srwLock);
    return nullptr;
}