#pragma once

#include <windows.h>
#include <dbghelp.h>
#include <tchar.h>
#include <strsafe.h>

// MSVC에서는 이 라인으로 링크를 자동 처리할 수 있다.
#pragma comment(lib, "dbghelp.lib")

namespace ErrorHandle
{
    const static uint32 STR_BUFFER_SIZE = 1024;

    // 가변 인자를 받아 포맷된 에러 메시지를 출력
    void ShowCrashLog(const wchar_t* Format, ...);
    // 미니덤프 생성자
    bool CreateMiniDump(EXCEPTION_POINTERS* pExceptionPointers);
    // pdb와 연동하여 디버그 로그 출력
    void PrintStackInfo(EXCEPTION_POINTERS* pExcpetionPointers);
    // UnHandled Exception Filter
    LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo);
};
