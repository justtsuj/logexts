#include "pch.h"
#include "util.h"
#include "Manifest/Manifest.h"
#include "FunctionWriter.h"

FunctionWriter::FunctionWriter() : pszDestEnd(callInfo), cchRemaining(4096){
    memset(callInfo, 0, sizeof(callInfo));
}

bool FunctionWriter::WritePointer(DWORD_PTR addr, bool notCheck) {
    StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT("%Ix"), addr);
    if (addr == NULL) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi))) {
        return (mbi.State == MEM_COMMIT) && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE));
    }
    return false;
}

bool IsStringPrintableA(LPCSTR str)
{
    //检查前50个字符
    for (int i = 0; i < 50; ++i) {
        if (!*(str + i)) return true;
        if (!isprint(*(str + i))) return false;
    }
    return true;
}

bool IsStringPrintableW(LPCWSTR str)
{
    //检查前50个字符
    for (int i = 0; i < 50; ++i) {
        if (!*(str + i)) return true;
        if (!iswprint(*(str + i))) return false;
    }
    return true;
}

void FunctionWriter::WriteString(int typeIndex, DWORD_PTR addr) {
    int unit = 0;
    size_t size = 0;
    if (typeIndex == 11) {
        unit = 1;
        if (!WritePointer(addr, 0)) return;
        if(IsStringPrintableA((LPCSTR)addr)) StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT(" \"%.100hs\""), (LPCSTR)addr);
        else StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT(" \"unknown char\""));
    }
    else if (typeIndex == 12 || typeIndex == 18) {
        unit = 2;
        if (!WritePointer(addr, 0)) return;
        if(IsStringPrintableW((LPCWSTR)addr)) StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT(" L\"%.100ls\""), (LPCWSTR)addr);
        else StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT(" L\"unknown char\""));
    }
}

void FunctionWriter::WriteGuid(DWORD_PTR addr) {

}

void FunctionWriter::WriteHString(DWORD_PTR addr) {

}

void FunctionWriter::WriteType(int typeIndex, DWORD_PTR addr) {
    switch (typeIndex)
    {
        case 1:
            break;
        case 2:
        case 6:
        case 9: // 1字节
            StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT("%x"), *(PBYTE)addr);
            break;
        case 3:
        case 10:    // 2字节
            StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT("%x"), *(PWORD)addr);
            break;
        case 4:
        case 5:
        case 7:
        case 15:    // 4字节
            StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT("%I32x"), *(PDWORD)addr);
            break;
        case 8: // 8字节
            StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT("%I64x"), *(PDWORD64)addr);
            break;
        case 11:    // 多字节字符串
        case 12:    // 宽字符字符串
            WriteString(typeIndex, *(PUINT_PTR)addr);
        case 14:    // guid
            WriteGuid(addr);
            break;
        case 18:    // HString
            WriteHString(*(PUINT_PTR)addr);
            break;
        default:
            break;
    }
}

void FunctionWriter::WriteStruct(ApiManifest::Declaration *paramDeclaration, DWORD_PTR addr) {
    DWORD_PTR tmpAddr = addr;
    ApiManifest::Struct *structt = paramDeclaration->GetType()->GetStruct();
    for (ApiManifest::Declaration* structMemberDeclaration : structt->declarations) {
        int size = structMemberDeclaration->GetSizeInMemory();
        for (INT32 i = 0; i < structMemberDeclaration->number; ++i) {
            // WriteValue(structMemberDeclaration, tmpAddr);
            tmpAddr += size;
        }
    }
}

void FunctionWriter::WriteValue(ApiManifest::Declaration* paramDeclaration, DWORD_PTR addr) {
    DWORD_PTR tmpAddr = addr;
    int typeIndex = paramDeclaration->GetType()->GetBaseType(true);
    for (INT32 i = 0; i < paramDeclaration->pointerRank; ++i) {  // 应该是自定义类型
        tmpAddr = *(PUINT_PTR)tmpAddr;
        if(!WritePointer(tmpAddr, 0)) return;
    }
    // 基础类型
    if (typeIndex == 13) WriteStruct(paramDeclaration, tmpAddr);
    else WriteType(typeIndex, tmpAddr);
}

DWORD_PTR FunctionWriter::LogHookGetNextParameterPointer(DWORD_PTR r_ebp, bool isFirst, int sizeOfParam) {
    DWORD_PTR result = 0;
#ifdef _WIN64
    if (isFirst) {
        result = r_ebp + 0x50;
    }
    else {
        result = *(PUINT_PTR)(r_ebp + 0x40);
    }
    *(PUINT_PTR)(r_ebp + 0x40) = result + sizeOfParam;
#else
    if (isFirst) {
        result = r_ebp + 0x28;
}
    else {
        result = *(PUINT_PTR)(r_ebp + 0xc);
    }
    *(PUINT_PTR)(r_ebp + 0xc) = result + sizeOfParam;
#endif // _WIN64
    return result;
}

void FunctionWriter::WriteEnter(LPCTSTR modName, DWORD_PTR offset, DWORD_PTR r_ebp, ApiManifest::Function* function) {

    // modName可以把后缀清理掉
    StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT("%s+%tx call %hs"), modName, offset, function->name.c_str());

    bool isFirst = true;
    for (ApiManifest::Declaration* declaration : function->parametersDeclarations) {
        DWORD_PTR nextParamPointer = LogHookGetNextParameterPointer(r_ebp, isFirst, declaration->GetSizeOnStack());
        if (!declaration->isOutParam) {
            StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT(", "));
            WriteValue(declaration, nextParamPointer);
        }
        isFirst = false;
    }
}

void FunctionWriter::WriteLeave(DWORD_PTR r_ebp, ApiManifest::Function* function, DWORD_PTR retValue) {
    //写返回值
    DWORD_PTR tmpRetValue = retValue;
    if (function->returnDeclaration->GetType()->GetBaseType(true) != 1 || function->returnDeclaration->pointerRank) {
        StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT(" return: "));
        WriteValue(function->returnDeclaration, (DWORD_PTR)&tmpRetValue);
    }
    //写输出参数
    bool isFirst = true;
    for (ApiManifest::Declaration* declaration : function->parametersDeclarations) {
        DWORD_PTR nextParamPointer = LogHookGetNextParameterPointer(r_ebp, isFirst, declaration->GetSizeOnStack());
        if (declaration->isOutParam) {
            StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT(", "));
            WriteValue(declaration, nextParamPointer);
        }
        isFirst = false;
    }
    //写结束段
    StringCchPrintfEx(pszDestEnd, cchRemaining, &pszDestEnd, &cchRemaining, NULL, TEXT("\r\n"));
    //输出
    OutputDebugString(callInfo);
}