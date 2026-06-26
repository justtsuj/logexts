#pragma once
class FunctionWriter
{
public:
	TCHAR callInfo[4096];
	LPTSTR pszDestEnd;
	size_t cchRemaining;

	FunctionWriter();

	void WriteType(int typeIndex, DWORD_PTR addr);
	void WriteValue(ApiManifest::Declaration* paramDeclaration, DWORD_PTR addr);
	DWORD_PTR LogHookGetNextParameterPointer(DWORD_PTR r_ebp, bool isFirst, int sizeOfParam);
	void WriteEnter(LPCTSTR modName, DWORD_PTR offset, DWORD_PTR r_ebp, ApiManifest::Function* function);
	void WriteLeave(DWORD_PTR r_ebp, ApiManifest::Function* function, DWORD_PTR retValue);
	void WriteString(int typeIndex, DWORD_PTR addr);
	bool WritePointer(DWORD_PTR addr, bool notCheck);
	void WriteGuid(DWORD_PTR addr);
	void WriteHString(DWORD_PTR addr);
	void WriteStruct(ApiManifest::Declaration* paramDeclaration, DWORD_PTR addr);
};