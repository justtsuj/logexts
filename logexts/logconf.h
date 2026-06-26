#pragma once
class LogConfig {
public:
	std::vector<std::string> excludeApis;
	std::vector<std::string> excludeDlls;
	TCHAR curDirp[MAX_PATH];
	HMODULE thisDll = NULL, ntdll = NULL, kernel32 = NULL, kernelBase = NULL;
	SYSTEM_INFO g_SysInfo;
	SRWLOCK g_srwLock;
	bool isDebug = false;
	// 添加一个开关用于控制是否打印非模块的api调用

	void ReadFromJSON(LPCTSTR filePath);
};

