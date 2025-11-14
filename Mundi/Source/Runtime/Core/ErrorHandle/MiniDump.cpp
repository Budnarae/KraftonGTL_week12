#include "pch.h"
#include "MiniDump.h"

// 가변 인자를 받아 포맷된 에러 메시지를 출력
void ErrorHandle::ShowCrashLog(const wchar_t* Format, ...)
{
    wchar_t Buffer[STR_BUFFER_SIZE];

    // 가변 인자 처리
    va_list args;
    va_start(args, Format);
    vswprintf_s(Buffer, STR_BUFFER_SIZE, Format, args);
    va_end(args);

    // 디버그 출력 및 메시지 박스로 표시
    OutputDebugStringW(Buffer);
    MessageBoxW(NULL, Buffer, L"Error", MB_OK | MB_ICONERROR);
}

// 간단한 미니덤프 생성자
bool ErrorHandle::CreateMiniDump(EXCEPTION_POINTERS* pExceptionPointers)
{
    WCHAR Path[MAX_PATH];
    WCHAR DumpFileName[MAX_PATH];
    const WCHAR* Version = L"v11.0";
    SYSTEMTIME LocalTime;

    // 현재 시간 가져오기
    GetLocalTime(&LocalTime);

    // 실행 파일의 경로 가져오기
    GetModuleFileName(NULL, Path, MAX_PATH);

    // 파일명 부분을 제거하여 디렉토리 경로만 남김
    WCHAR* lastSlash = wcsrchr(Path, L'\\');
    if (lastSlash != NULL)
    {
        *lastSlash = L'\0';
    }

    // ErrorLog 디렉토리 생성
    StringCchPrintf(DumpFileName, MAX_PATH, L"%s\\ErrorLog", Path);
    CreateDirectory(DumpFileName, NULL);

    // ErrorLog\MiniDump 디렉토리 생성
    StringCchPrintf(DumpFileName, MAX_PATH, L"%s\\ErrorLog\\MiniDump", Path);
    CreateDirectory(DumpFileName, NULL);

    // 파일명 생성: ErrorLog\MiniDump\{Version}-{YYYYMMDD}-{HHMMSS}-{ProcessId}-{ThreadId}.dmp
    StringCchPrintf(
        DumpFileName,
        MAX_PATH,
        L"%s\\ErrorLog\\MiniDump\\%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
        Path,
        Version,
        LocalTime.wYear,
        LocalTime.wMonth,
        LocalTime.wDay,
        LocalTime.wHour,
        LocalTime.wMinute,
        LocalTime.wSecond,
        GetCurrentProcessId(),
        GetCurrentThreadId()
    );

    HANDLE hFile = CreateFileW
    (
        DumpFileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowCrashLog(L"CreateFileW failed, err=%u\n", GetLastError());
        return false;
    }

    // MINIDUMP_EXCEPTION_INFORMATION 구조체 설정
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;
    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = pExceptionPointers;
    ExpParam.ClientPointers = TRUE;

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
        (pExceptionPointers ? &ExpParam : NULL),
        NULL,
        NULL
    );

    CloseHandle(hFile);

    if (!success)
    {
        ShowCrashLog(L"MiniDumpWriteDump failed, err=%u\n", GetLastError());
        return false;
    }

    ShowCrashLog(L"Minidump created: %s\n", DumpFileName);
    return true;
}

// pdb와 연동하여 디버그 로그를 파일로 출력
void ErrorHandle::PrintStackInfo(EXCEPTION_POINTERS* pExcpetionPointers)
{
    WCHAR Path[MAX_PATH];
    WCHAR LogFileName[MAX_PATH];
    const WCHAR* Version = L"v11.0";
    SYSTEMTIME LocalTime;

    // 현재 시간 가져오기
    GetLocalTime(&LocalTime);

    // 실행 파일의 경로 가져오기
    GetModuleFileName(NULL, Path, MAX_PATH);

    // 파일명 부분을 제거하여 디렉토리 경로만 남김
    WCHAR* lastSlash = wcsrchr(Path, L'\\');
    if (lastSlash != NULL)
    {
        *lastSlash = L'\0';
    }

    // ErrorLog 디렉토리 생성
    StringCchPrintf(LogFileName, MAX_PATH, L"%s\\ErrorLog", Path);
    CreateDirectory(LogFileName, NULL);

    // ErrorLog\Log 디렉토리 생성
    StringCchPrintf(LogFileName, MAX_PATH, L"%s\\ErrorLog\\Log", Path);
    CreateDirectory(LogFileName, NULL);

    // 파일명 생성: ErrorLog\Log\{Version}-{YYYYMMDD}-{HHMMSS}-{ProcessId}-{ThreadId}.log
    StringCchPrintf(
        LogFileName,
        MAX_PATH,
        L"%s\\ErrorLog\\Log\\%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.log",
        Path,
        Version,
        LocalTime.wYear,
        LocalTime.wMonth,
        LocalTime.wDay,
        LocalTime.wHour,
        LocalTime.wMinute,
        LocalTime.wSecond,
        GetCurrentProcessId(),
        GetCurrentThreadId()
    );

    // 로그 파일 열기
    FILE* logFile = nullptr;
    if (_wfopen_s(&logFile, LogFileName, L"w, ccs=UTF-8") != 0 || logFile == nullptr)
    {
        ShowCrashLog(L"Failed to create log file: %s\n", LogFileName);
        return;
    }

    // 로그 헤더 작성
    fwprintf(logFile, L"========================================\n");
    fwprintf(logFile, L"Crash Log\n");
    fwprintf(logFile, L"Time: %04d-%02d-%02d %02d:%02d:%02d\n",
        LocalTime.wYear, LocalTime.wMonth, LocalTime.wDay,
        LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond);
    fwprintf(logFile, L"Process ID: %ld\n", GetCurrentProcessId());
    fwprintf(logFile, L"Thread ID: %ld\n", GetCurrentThreadId());
    fwprintf(logFile, L"========================================\n\n");

    // 스택 정보 수집
    HANDLE hProcess = GetCurrentProcess();
    SymInitialize(hProcess, NULL, TRUE);

    CONTEXT context = *pExcpetionPointers->ContextRecord;
    STACKFRAME64 stackFrame;
    ZeroMemory(&stackFrame, sizeof(STACKFRAME64));

#ifdef _M_X64
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#else
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Offset = context.Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#endif

    fwprintf(logFile, L"Callstack:\n");

    for (int i = 0; i < 20; i++)
    {
        if (!StackWalk64(
            machineType,
            hProcess,
            GetCurrentThread(),
            &stackFrame,
            &context,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        )) break;

        DWORD64 address = stackFrame.AddrPC.Offset;
        if (address == 0) break;

        char buffer[sizeof(SYMBOL_INFO) + 256];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = 255;

        if (SymFromAddr(hProcess, address, 0, pSymbol))
        {
            IMAGEHLP_LINE64 line;
            DWORD dwDisplacement = 0;
            ZeroMemory(&line, sizeof(line));
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            if (SymGetLineFromAddr64(
                hProcess,
                address,
                &dwDisplacement,
                &line))
            {
                fwprintf(logFile, L"  [%d] %S - %S:%lu\n", i, pSymbol->Name, line.FileName, line.LineNumber);
            }
            else
            {
                fwprintf(logFile, L"  [%d] %S - 0x%llx\n", i, pSymbol->Name, pSymbol->Address);
            }
        }
        else
        {
            fwprintf(logFile, L"  [%d] 0x%llx\n", i, address);
        }
    }

    SymCleanup(hProcess);

    // 로그 파일 닫기
    fclose(logFile);

    ShowCrashLog(L"Stack trace log created: %s\n", LogFileName);
}

// UnHandled Exception Filter
LONG WINAPI ErrorHandle::UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo)
{
    ShowCrashLog(L"Exception occured.\n");

    // 예외 포인터를 사용하여
    // 1. MiniDump 생성
    CreateMiniDump(pExceptionInfo);

    // 2. PDB 연동하여 콜스택 출력
    PrintStackInfo(pExceptionInfo);

    // 처리 후 기본 동작으로 전달 (프로세스 종료)
    return EXCEPTION_EXECUTE_HANDLER;
}