#pragma once

#include "SViewerWindowBase.h"
#include "Source/Runtime/Engine/AnimationViewer/AnimationViewerState.h"
#include "Source/Slate/Widgets/BoneHierarchyWidget.h"
#include "Source/Slate/Widgets/BonePropertyEditor.h"
#include "Source/Slate/Widgets/NotifyPropertyEditor.h"

class FViewport;
class FViewportClient;
class UWorld;
struct ID3D11Device;

/**
 * 애니메이션 뷰어 윈도우
 *
 * 기능:
 * - 스켈레탈 메시 & 애니메이션 로드
 * - 본 계층 구조 표시 (BoneHierarchyWidget 재사용!)
 * - 타임라인 및 재생 컨트롤
 * - 재생/일시정지/정지/루프
 */
class SAnimationViewerWindow : public SViewerWindowBase
{
public:
    SAnimationViewerWindow();
    virtual ~SAnimationViewerWindow();

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

    // Load assets
    void LoadSkeletalMesh(const FString& Path);
    void LoadAnimation(const FString& Path);

private:
    AnimationViewerState* State = nullptr;
    // Layout state
    float LeftPanelRatio = 0.25f;
    float RightPanelRatio = 0.25f;
    float BottomPanelHeight = 250.0f;  // 타임라인 + 노티파이 트랙 높이

    // Cached regions
    FRect CenterRect;

    // UI Widgets
    FBoneHierarchyWidget BoneHierarchy;
    FBonePropertyEditor PropertyEditor;
    FNotifyPropertyEditor NotifyPropertyEditor;

    // 본 선택 상태 (BoneHierarchyWidget용)
    int32 SelectedBoneIndex = -1;
    std::set<int32> ExpandedBoneIndices;

    // 드롭다운 선택 상태
    int32 SelectedMeshIndex = -1;
    int32 SelectedAnimIndex = -1;

    // 컨텍스트 메뉴 상태
    float PendingNotifyTime = 0.0f;

    // 선택된 Notify
    UAnimNotify* SelectedNotify = nullptr;

    // 헬퍼 함수
    void UpdateBoneTransformFromSkeleton(AnimationViewerState* State);
    void ApplyBoneTransform(AnimationViewerState* State);
    void ExpandToSelectedBone(AnimationViewerState* State, int32 BoneIndex);
};
