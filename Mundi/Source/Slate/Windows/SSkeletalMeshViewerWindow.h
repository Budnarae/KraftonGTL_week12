#pragma once
#include "SViewerWindowBase.h"
#include "Source/Runtime/Engine/SkeletalViewer/ViewerState.h"

class FViewport;
class FViewportClient;
class UWorld;
struct ID3D11Device;

class SSkeletalMeshViewerWindow : public SViewerWindowBase
{
public:
    SSkeletalMeshViewerWindow();
    virtual ~SSkeletalMeshViewerWindow();

    // 베이스 클래스의 Initialize 오버라이드 (통합 API용)
    virtual bool Initialize(ID3D11Device* InDevice, UWorld* InWorld) override;

    // 커스텀 Initialize (위치/크기 지정)
    bool Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice);

    // SViewerWindowBase overrides
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
        ViewerState* State = static_cast<ViewerState*>(ActiveState);
        return State ? State->Viewport : nullptr;
    }
    FViewportClient* GetViewportClient() const
    {
        ViewerState* State = static_cast<ViewerState*>(ActiveState);
        return State ? State->Client : nullptr;
    }

    // Load a skeletal mesh into the active tab
    void LoadSkeletalMesh(const FString& Path);

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

private:
    void UpdateBoneTransformFromSkeleton(ViewerState* State);
    void ApplyBoneTransform(ViewerState* State);

    void ExpandToSelectedBone(ViewerState* State, int32 BoneIndex);
};
