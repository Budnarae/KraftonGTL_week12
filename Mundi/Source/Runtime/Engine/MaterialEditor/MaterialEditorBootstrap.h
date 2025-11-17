#pragma once

#include "MaterialEditorState.h"

struct ID3D11Device;
class UWorld;

/**
 * 머티리얼 에디터 탭 상태 생성/삭제 팩토리
 */
class MaterialEditorBootstrap
{
public:
    /**
     * 탭 상태를 생성합니다.
     */
    static MaterialEditorState* CreateEditorState(
        const char* TabName,
        UWorld* InWorld,
        ID3D11Device* InDevice
    );

    /**
     * 탭 상태를 삭제합니다.
     */
    static void DestroyEditorState(MaterialEditorState*& State);
};
