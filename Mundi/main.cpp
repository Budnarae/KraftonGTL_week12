#include "pch.h"
#include "EditorEngine.h"
#include "Source/Runtime/Core/ErrorHandle/ErrorHandle.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <cstdlib>
#   include <crtdbg.h>
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetBreakAlloc(0);
#endif
    // 전역 예외 필터 등록: 프로그램에서 잡히지 않은 예외가 발생하면 호출된다.
    SetUnhandledExceptionFilter(ErrorHandle::UnhandledExceptionFilter);
    
    if (!GEngine.Startup(hInstance))
        return -1;

    GEngine.MainLoop();
    GEngine.Shutdown();

    return 0;
}
