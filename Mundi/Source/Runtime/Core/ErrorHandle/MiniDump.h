#pragma once

#include <windows.h>
#include <dbghelp.h>
#include <tchar.h>
#include <strsafe.h>

// MSVC에서는 이 라인으로 링크를 자동 처리할 수 있다.
#pragma comment(lib, "dbghelp.lib")

namespace ErrorHandle
{
    // 간단한 미니덤프 생성자
    bool CreateMiniDump(EXCEPTION_POINTERS* pExceptionPointers);
    // UnHandled Exception Filter
    LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo);
};
