#pragma once

//class SharedBlock
//{
//private:
//	/* data */
//public:
//	SharedBlock(/* args */);
//	~SharedBlock();
//};

struct LogirInfo{
	CONTEXT context;
	LPVOID injectDllSharedMemCode;
};

typedef HMODULE (__stdcall *LoadLibraryWPROC)(LPCWSTR lpLibFileName);
typedef FARPROC(__stdcall *GetProcAddressPROC)(HMODULE hModule, LPCSTR lpProcName);

struct InjectParameter
{
	LoadLibraryWPROC loadLibraryWPROC = nullptr;
	GetProcAddressPROC getProcAddressPROC = nullptr;
	CHAR functionName[20] = "InjectionCleanup";
	TCHAR logextMoudleName[MAX_PATH] = {};
	HMODULE hModule;
	int injectionCleanupOffset = 0;
};

class DebuggeeInfo
{
public:
	DebuggeeInfo* next = nullptr;
	// SharedBlock *sharedBlock = nullptr;
	HANDLE hProcess = NULL;
	HANDLE hThread = NULL;
	int injectMode = 0;
	TCHAR mainMoudleName[MAX_PATH] = TEXT("<unknown>");
	LogirInfo logirInfo;	// 原程序在这使用的指针

	bool InjectLogexts();
	bool InjectThisDll();
	bool InitInjectionData(InjectParameter* injectParameter);
	int ResumeFromInjection(HANDLE hCurrentThread);
};
typedef DebuggeeInfo* PDebuggeeInfo;

bool LogextsAttach(HMODULE hModule);
void LogextsDetach();