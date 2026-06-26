#pragma once
namespace ApiLogFile {

	typedef struct _LDR_DATA_TABLE_ENTRY {
		LIST_ENTRY InLoadOrderLinks;
		LIST_ENTRY InMemoryOrderLinks;
		LIST_ENTRY InInitializationOrderLinks;
		PVOID DllBase;
		PVOID EntryPoint;
		ULONG SizeOfImage;
		UNICODE_STRING FullDllName;
		UNICODE_STRING BaseDllName;
		ULONG Flags;
		SHORT LoadCount;
		SHORT TlsIndex;
		LIST_ENTRY HashLinks;
		PVOID SectionPointer;
		ULONG CheckSum;
		ULONG TimeDateStamp;
		PVOID LoadedImports;
		PVOID EntryPointActivationContext;
		PVOID PatchInformation;
		LIST_ENTRY ForwarderLinks;
		LIST_ENTRY ServiceTagLinks;
		LIST_ENTRY StaticLinks;
		PVOID ContextInformation;
		ULONG OriginalBase;
		LARGE_INTEGER LoadTime;
	} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

	class LogModule
	{
	public:
		PBYTE modBaseAddr;
		DWORD modImageSize;
		TCHAR path[MAX_PATH] = {};
		LPCTSTR modName = nullptr;
		DWORD flag;

		LogModule(/* args */);
		LogModule(PBYTE _modBaseAddr, DWORD _modBaseSize, LPCTSTR _path);
		~LogModule();
		DWORD GetOffsetFromAddress(PBYTE addr);
	};

	class ModuleList
	{
	public:
		std::vector<LogModule*> curModuleList, allModuleList;

		ModuleList(/* args */);
		~ModuleList();
		LogModule* CheckAddAModule(MODULEENTRY32& mi);
		LogModule* FindModule(MODULEENTRY32& mi);
		LogModule* FindModule(PLDR_DATA_TABLE_ENTRY pEntry);
		bool TakeSnapshot(std::vector<LogModule*>& newModuleList, std::vector<LogModule*>& reloadModuleList);
		LogModule* GetModuleFromAddress(PBYTE addr);
	};
}