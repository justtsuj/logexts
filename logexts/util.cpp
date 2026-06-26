#include "pch.h"

void Error(LPCTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s\r\n"),
        lpszFunction, dw, lpMsgBuf);
    OutputDebugString((LPCTSTR)lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

void QueryMemory(LPCVOID lpAddress) {
    MEMORY_BASIC_INFORMATION mbi;
    // 使用VirtualQuery查询内存访问权限
    if (VirtualQuery(lpAddress, &mbi, sizeof(mbi)) != 0) {
        // 检查是否可读
        if ((mbi.Protect & PAGE_READONLY) || (mbi.Protect & PAGE_READWRITE) || (mbi.Protect & PAGE_EXECUTE_READ) || (mbi.Protect & PAGE_EXECUTE_READWRITE)) {
            std::cout << "Memory is readable" << std::endl;
        }
        else {
            std::cout << "Memory is not readable" << std::endl;
        }

        // 检查是否可写
        if ((mbi.Protect & PAGE_READWRITE) || (mbi.Protect & PAGE_WRITECOPY) || (mbi.Protect & PAGE_EXECUTE_READWRITE)) {
            std::cout << "Memory is writable" << std::endl;
        }
        else {
            std::cout << "Memory is not writable" << std::endl;
        }

        // 检查是否可执行
        if ((mbi.Protect & PAGE_EXECUTE) || (mbi.Protect & PAGE_EXECUTE_READ) || (mbi.Protect & PAGE_EXECUTE_READWRITE)) {
            std::cout << "Memory is executable" << std::endl;
        }
        else {
            std::cout << "Memory is not executable" << std::endl;
        }
    }
}

void DebugOut(LPCTSTR fmt, ...)
{
    TCHAR dbg_out[4096];
    va_list argp;
    va_start(argp, fmt);
    StringCchVPrintf(dbg_out, 4096, fmt, argp);
    va_end(argp);
    OutputDebugString(dbg_out);
}