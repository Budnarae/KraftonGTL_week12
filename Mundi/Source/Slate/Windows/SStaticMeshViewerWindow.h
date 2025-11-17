#pragma once

#include "SViewerWindowBase.h"
#include "Source/Runtime/Engine/StaticMeshViewer/StaticMeshViewerState.h"
#include "Source/Slate/Widgets/AssetBrowserWidget.h"

class FViewport;
class FViewportClient;
class UWorld;
struct ID3D11Device;

/**
 * 스태틱 메시 뷰어 윈도우
 *
 * 기능:
 * - FBX/OBJ 파일 로드
 * - LOD 레벨 선택
 * - 머티리얼 슬롯 표시
 * - Wireframe/Bounds/Collision 표시
 */
class SStaticMeshViewerWindow : public SViewerWindowBase
{
public:
    SStaticMeshViewerWindow();
    virtual ~SStaticMeshViewerWindow();

    // SViewerWindowBase overrides
    virtual bool Initialize(ID3D11Device* InDevice, UWorld* InWorld) override;
    virtual void LoadAsset(const FString& AssetPath) override;

    // SWindow overrides
    virtual void OnRender() override;
    virtual void OnUpdate(float DeltaSeconds) override;
    virtual void OnMouseMove(FVector2D MousePos) override;
    virtual void OnMouseDown(FVector2D MousePos, uint32 Button) override;
    virtual void OnMouseUp(FVector2D MousePos, uint32 Button) override;

    void OnRenderViewport();

    // Accessors (active tab)
    FViewport* GetViewport() const
    {
        StaticMeshViewerState* State = static_cast<StaticMeshViewerState*>(ActiveState);
        return State ? State->Viewport : nullptr;
    }

    FViewportClient* GetViewportClient() const
    {
        StaticMeshViewerState* State = static_cast<StaticMeshViewerState*>(ActiveState);
        return State ? State->Client : nullptr;
    }

    // Load a static mesh into the active tab
    void LoadStaticMesh(const FString& Path);

protected:
    // SViewerWindowBase overrides (탭 관리)
    virtual ViewerTabStateBase* CreateTabState(const char* Name) override;
    virtual void DestroyTabState(ViewerTabStateBase* State) override;

private:
    // Layout state
    float LeftPanelRatio = 0.25f;   // 25% of width
    float RightPanelRatio = 0.25f;  // 25% of width

    // Cached center region used for viewport sizing and input mapping
    FRect CenterRect;

    // UI Widgets
    FAssetBrowserWidget AssetBrowser;
};
