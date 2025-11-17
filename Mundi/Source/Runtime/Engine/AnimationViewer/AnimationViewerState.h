#pragma once
#include "Windows/SViewerWindowBase.h"


class UWorld;
class FViewport;
class FViewportClient;
class ASkeletalMeshActor;
class USkeletalMesh;
class UAnimationSequence;

/**
 * 애니메이션 뷰어의 탭별 상태
 */
class AnimationViewerState : public ViewerTabStateBase
{
public:
    AnimationViewerState() = default;
    ~AnimationViewerState() = default;

    // 프리뷰 월드 및 뷰포트
    UWorld* World = nullptr;
    FViewport* Viewport = nullptr;
    FViewportClient* Client = nullptr;

    // 프리뷰 액터
    ASkeletalMeshActor* PreviewActor = nullptr;

    // 현재 로드된 에셋
    USkeletalMesh* CurrentMesh = nullptr;
    UAnimationSequence* CurrentAnimation = nullptr;
    FString LoadedMeshPath;
    FString LoadedAnimPath;

    // UI 상태
    char MeshPathBuffer[260] = {};
    char AnimPathBuffer[260] = {};

    // 애니메이션 재생 상태
    bool bIsPlaying = false;
    float CurrentTime = 0.0f;
    float PlaybackSpeed = 1.0f;
    bool bLooping = true;

    // 본 표시 및 편집 관련
    bool bShowMesh = true;
    bool bShowBones = true;
    bool bBoneLinesDirty = true;
    int32 LastSelectedBoneIndex = -1;

    // 본 트랜스폼 편집 관련
    FVector EditBoneLocation;
    FVector EditBoneRotation;  // Euler angles in degrees
    FVector EditBoneScale;
    bool bBoneTransformChanged = false;
    bool bBoneRotationEditing = false;
};
