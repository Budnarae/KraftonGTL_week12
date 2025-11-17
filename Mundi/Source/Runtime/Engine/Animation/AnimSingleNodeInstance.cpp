#include "pch.h"
#include "AnimSingleNodeInstance.h"
#include "SkeletalMeshComponent.h"
#include "AnimationSequence.h"
#include "SkeletalMesh.h"

IMPLEMENT_CLASS(UAnimSingleNodeInstance)

UAnimSingleNodeInstance::~UAnimSingleNodeInstance()
{
    // CurrentAnimation은 ResourceManager가 관리하는 리소스이므로 delete하지 않음
    CurrentAnimation = nullptr;
}

void UAnimSingleNodeInstance::SetAnimation(UAnimationAsset* NewAnimation)
{
    CurrentAnimation = NewAnimation;
}

//Animation Helper
void UAnimSingleNodeInstance::PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping)
{
    if (!OwnerSkeletalComp || !NewAnimToPlay)
    {
        return;
    }

    OwnerSkeletalComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    SetAnimation(NewAnimToPlay);

    // AnimationSequence인 경우 추가 설정
    UAnimationSequence* AnimSequence = dynamic_cast<UAnimationSequence*>(NewAnimToPlay);
    if (AnimSequence)
    {
        // Skeleton 설정 (SkeletalMeshComponent로부터 가져옴)
        USkeletalMesh* SkeletalMesh = OwnerSkeletalComp->GetSkeletalMesh();
        if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshData())
        {
            const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
            AnimSequence->SetSkeleton(Skeleton);
        }

        // 루핑 및 재생 시간 설정
        AnimSequence->SetLooping(bLooping);
        AnimSequence->ResetPlaybackTime();
    }
}

void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    if (!CurrentAnimation)
        return;

    FAnimationUpdateContext Context;
    Context.DeltaTime = DeltaSeconds;
    CurrentAnimation->Update(Context);
}

void UAnimSingleNodeInstance::EvaluateAnimation()
{
    if (!CurrentAnimation || !OwnerSkeletalComp)
        return;

    USkeletalMesh* SkeletalMesh = OwnerSkeletalComp->GetSkeletalMesh();
    const FSkeleton* Skeleton = (SkeletalMesh && SkeletalMesh->GetSkeletalMeshData()) ?
        &SkeletalMesh->GetSkeletalMeshData()->Skeleton : nullptr;

    FPoseContext CurrentPose(Skeleton);
    CurrentAnimation->Evaluate(CurrentPose);

    // 평가된 포즈를 SkeletalMeshComponent에 적용
    if (CurrentPose.EvaluatedPoses.Num() > 0)
    {
        TArray<FTransform>& LocalPose = OwnerSkeletalComp->GetLocalSpacePose();
        LocalPose = CurrentPose.EvaluatedPoses;
        OwnerSkeletalComp->ForceRecomputePose();
    }
}
