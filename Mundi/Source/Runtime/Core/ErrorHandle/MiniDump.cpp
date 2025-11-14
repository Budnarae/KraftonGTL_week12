#include "pch.h"
#include "MiniDump.h"

// 간단한 미니덤프 생성자
bool ErrorHandle::CreateMiniDump(EXCEPTION_POINTERS* pExceptionPointers)
{
    // TODO: 추후 시간/쓰레드 ID 등으로 고유화
    const wchar_t* DumpFileName = L"crash.dmp";

    HANDLE hFile = CreateFileW
    (
        DumpFileName,
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        wchar_t buffer[1024];
        swprintf_s(buffer, L"CreateFileW failed, err=%u\n", err);
        OutputDebugStringW(buffer);
        MessageBoxW(NULL, buffer, L"Error", MB_OK | MB_ICONERROR);
        
        return false;
    }

    // MINIDUMP_EXCEPTION_INFORMATION 구조체 설정
    MINIDUMP_EXCEPTION_INFORMATION mdei;
    mdei.ThreadId = GetCurrentThreadId();
    mdei.ExceptionPointers = pExceptionPointers;
    // 프로세스 내부 포인터 사용 여부
    mdei.ClientPointers = FALSE;

    // 포함할 정보 타입 지정 (필요에 따라 변경)
    MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(
        MiniDumpNormal |
        MiniDumpWithDataSegs |
        MiniDumpWithHandleData
    );

    BOOL success = MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        dumpType,
        (pExceptionPointers ? &mdei : NULL),
        NULL,
        NULL
    );

    CloseHandle(hFile);

    if (!success)
    {
        DWORD err = GetLastError();
        wchar_t buffer[1024];
        swprintf_s(buffer, L"MiniDumpWriteDump failed, err=%u\n", err);
        OutputDebugStringW(buffer);
        MessageBoxW(NULL, buffer, L"Error", MB_OK | MB_ICONERROR);
        
        return false;
    }

    wchar_t buffer[1024];
    swprintf_s(buffer, L"Minidump created: %s\n", DumpFileName);
    OutputDebugStringW(buffer);
    MessageBoxW(NULL, buffer, L"Error", MB_OK | MB_ICONERROR);
    
    return true;
}

// UnHandled Exception Filter
LONG WINAPI ErrorHandle::MyUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo)
{
    const wchar_t* ExceptionMsg = L"Exception occured.\n";
    OutputDebugStringW(ExceptionMsg);
    MessageBoxW(NULL, ExceptionMsg, L"Error", MB_OK | MB_ICONERROR);
        
    // 예외 포인터를 사용해 미니덤프를 기록
    CreateMiniDump(pExceptionInfo);

    // 추가 동작: 로그, 네트워크 업로드, 사용자 알림 등

    // 처리 후 기본 동작으로 전달 (프로세스 종료)
    return EXCEPTION_EXECUTE_HANDLER;
}