#pragma once

#include "StaticMeshViewerState.h"

struct ID3D11Device;
class UWorld;

/**
 * 스태틱 메시 뷰어 탭 상태 생성/삭제 팩토리
 */
class StaticMeshViewerBootstrap
{
public:
    /**
     * 탭 상태를 생성합니다.
     */
    static StaticMeshViewerState* CreateViewerState(
        const char* TabName,
        UWorld* InWorld,
        ID3D11Device* InDevice
    );

    /**
     * 탭 상태를 삭제합니다.
     */
    static void DestroyViewerState(StaticMeshViewerState*& State);
};
