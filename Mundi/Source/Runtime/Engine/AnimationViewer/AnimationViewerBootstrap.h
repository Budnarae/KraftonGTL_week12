#pragma once

#include "AnimationViewerState.h"

struct ID3D11Device;
class UWorld;

/**
 * 애니메이션 뷰어 탭 상태 생성/삭제 팩토리
 */
class AnimationViewerBootstrap
{
public:
    static AnimationViewerState* CreateViewerState(
        const char* TabName,
        UWorld* InWorld,
        ID3D11Device* InDevice
    );

    static void DestroyViewerState(AnimationViewerState*& State);
};
