#include "pch.h"
#include "AnimInstance.h"
#include "SkeletalMeshComponent.h"
#include "AnimationAsset.h"

IMPLEMENT_CLASS(UAnimInstance)
void UAnimInstance::UpdateAnimation(float DeltaTime)
{
    if (!OwnerSkeletalComp || !CurrentAnimation || !bIsPlaying)
    {
        return;
    }
        

    USkeletalMesh* SkeletalMesh = OwnerSkeletalComp->GetSkeletalMesh();
    if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData())
    {
        return;
    }
    
    // 1) 시간 업데이트
    CurrentAnimationTime += DeltaTime;
    float PlayLength = CurrentAnimation->GetPlayLength();
    
    if (CurrentAnimationTime >= PlayLength)
    {
        if (bIsLooping)
        {
            CurrentAnimationTime = fmod(CurrentAnimationTime, PlayLength);
        }
        else
        {
            CurrentAnimationTime = PlayLength;
            bIsPlaying = false;
        }
    }

    // 2) 모든 본의 로컬 포즈 업데이트
    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    const int32 NumBones = Skeleton.Bones.Num();

    TArray<FTransform>& LocalPose = OwnerSkeletalComp->GetLocalSpacePose();
    LocalPose.SetNum(NumBones);
    
    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FBone& Bone = Skeleton.Bones[BoneIndex];
        FName BoneName = FName(Bone.Name);

        // UAnimationAsset 인터페이스로 본 포즈 가져오기
        // (AnimSequence, Montage 등 모든 타입 지원)
        FTransform AnimPose = CurrentAnimation->GetBonePose(BoneName, CurrentAnimationTime);
        LocalPose[BoneIndex] = AnimPose;
    }

    // 포즈 재계산 => 스키닝
    OwnerSkeletalComp->ForceRecomputePose();
}

void UAnimInstance::SetAnimation(UAnimationAsset* NewAnimation)
{
    CurrentAnimation = NewAnimation;
    CurrentAnimationTime = 0.0f;
}

void UAnimInstance::Play(bool bLooping)
{
    bIsPlaying = true;
    bIsLooping = bLooping;
    CurrentAnimationTime = 0.0f;
}

//Animation Helper
void UAnimInstance::PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping)
{
    if (!OwnerSkeletalComp || !NewAnimToPlay)
    {
        return;
    }

    OwnerSkeletalComp->SetAnimationMode(USkeletalMeshComponent::EAnimationMode::AnimationSingleNode);
    SetAnimation(NewAnimToPlay);
    Play(bLooping);
}

void UAnimInstance::StopAnimation()
{
    bIsPlaying = false;
    CurrentAnimationTime = 0.0f;
}

void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    FAnimationUpdateContext Context;
    Context.DeltaTime = DeltaSeconds;

    ASM.Update(Context);
}

void UAnimInstance::EvaluateAnimation()
{
    ASM.Evaluate(CurrentPose);
}

FPoseContext& UAnimInstance::GetCurrentPose()
{
    return CurrentPose;
}
