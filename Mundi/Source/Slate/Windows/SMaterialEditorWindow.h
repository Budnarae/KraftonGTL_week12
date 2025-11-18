#pragma once

#include "SViewerWindowBase.h"
#include "Source/Runtime/Engine/MaterialEditor/MaterialEditorState.h"
#include "Source/Slate/Widgets/AssetBrowserWidget.h"

class FViewport;
class FViewportClient;
class UWorld;
struct ID3D11Device;

/**
 * 머티리얼 에디터 윈도우
 *
 * 기능:
 * - 머티리얼 로드 및 편집
 * - 파라미터 실시간 조정 (Scalar, Vector, Texture)
 * - 프리뷰 메시 선택 (Sphere, Cube, Cylinder, Plane)
 */
class SMaterialEditorWindow : public SViewerWindowBase
{
public:
    SMaterialEditorWindow();
    virtual ~SMaterialEditorWindow();

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

    // Accessors
    FViewport* GetViewport() const
    {
        return State ? State->Viewport : nullptr;
    }

    FViewportClient* GetViewportClient() const
    {
        return State ? State->Client : nullptr;
    }

    // Load a material
    void LoadMaterial(const FString& Path);

private:
    MaterialEditorState* State = nullptr;
    // Layout state
    float LeftPanelRatio = 0.25f;
    float RightPanelRatio = 0.30f;

    // Cached center region
    FRect CenterRect;

    // UI Widgets
    FAssetBrowserWidget AssetBrowser;
};
