#include "pch.h"
#include "SkeletalMeshComponent.h"
#include "../Animation/AnimInstance.h"
#include "Source/Runtime/Engine/Animation/AnimationAsset.h"

USkeletalMeshComponent::USkeletalMeshComponent()
{
    // 테스트용 기본 메시 설정
    SetSkeletalMesh(GDataDir + "/Test.fbx");
}

void USkeletalMeshComponent::OnRegister(UWorld* InWorld)
{
    Super::OnRegister(InWorld);

    if (AnimInstance && AnimInstance->GetSkeletalComponent() != this)
    {
        AnimInstance = nullptr;
    }

    if (!AnimInstance)
    {
        UAnimInstance* NewAnimInstanceClass = NewObject<UAnimInstance>();
        SetAnimInstanceClass(NewAnimInstanceClass);
    }

    if (AnimInstance)
    {
        FPoseContext Start, End, Blended;

        // FOR TEST
        SeqA = RESOURCE.Load<UAnimationSequence>("Standard Run_mixamo.com");
        SeqB = RESOURCE.Load<UAnimationSequence>("Standard Walk_mixamo.com");

        AnimInstance->PlayBlendedAnimation(*SeqA, *SeqB);
    }
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    if (AnimationMode == EAnimationMode::AnimationSingleNode && AnimInstance)
    {
        // FOR TEST
        AnimInstance->UpdateBlendedAnimation(DeltaTime, Alpha);
        AnimInstance->UpdateAnimation(DeltaTime);
    }

    // // FOR TEST ////
    //  if (!SkeletalMesh) { return; }
    //  constexpr int32 TEST_BONE_INDEX = 2;
    //  if (!bIsInitialized)
    //  {
    //      TestBoneBasePose = CurrentLocalSpacePose[TEST_BONE_INDEX];
    //      bIsInitialized = true;
    //  }
    //  TestTime += DeltaTime;
    //  float Angle = sinf(TestTime * 2.f);
    //  FQuat TestRotation = FQuat::FromAxisAngle(FVector(1.f, 0.f, 0.f), Angle);
    //  TestRotation.Normalize();
    //  FTransform NewLocalPose = TestBoneBasePose;
    //  NewLocalPose.Rotation = TestRotation * TestBoneBasePose.Rotation;
    //  SetBoneLocalTransform(TEST_BONE_INDEX, NewLocalPose);
    // // FOR TEST ////
}

void USkeletalMeshComponent::SetSkeletalMesh(const FString& PathFileName)
{
    Super::SetSkeletalMesh(PathFileName);

    if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshData())
    {
        const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
        const int32 NumBones = Skeleton.Bones.Num();

        CurrentLocalSpacePose.SetNum(NumBones);
        CurrentComponentSpacePose.SetNum(NumBones);
        TempFinalSkinningMatrices.SetNum(NumBones);
        TempFinalSkinningNormalMatrices.SetNum(NumBones);

        for (int32 i = 0; i < NumBones; ++i)
        {
            const FBone& ThisBone = Skeleton.Bones[i];
            const int32 ParentIndex = ThisBone.ParentIndex;
            FMatrix LocalBindMatrix;

            if (ParentIndex == -1) // 루트 본
            {
                LocalBindMatrix = ThisBone.BindPose;
            }
            else // 자식 본
            {
                const FMatrix& ParentInverseBindPose = Skeleton.Bones[ParentIndex].InverseBindPose;
                LocalBindMatrix = ThisBone.BindPose * ParentInverseBindPose;
            }
            // 계산된 로컬 행렬을 로컬 트랜스폼으로 변환
            CurrentLocalSpacePose[i] = FTransform(LocalBindMatrix); 
        }
        
        ForceRecomputePose(); 
    }
    else
    {
        // 메시 로드 실패 시 버퍼 비우기
        CurrentLocalSpacePose.Empty();
        CurrentComponentSpacePose.Empty();
        TempFinalSkinningMatrices.Empty();
        TempFinalSkinningNormalMatrices.Empty();
    }
}

void USkeletalMeshComponent::SetAnimInstanceClass(UAnimInstance* NewAnimInstanceClass)
{
    if (AnimInstance && AnimInstance->GetSkeletalComponent() == this)
    {
        DeleteObject(AnimInstance);
        AnimInstance = nullptr;
    }

    AnimInstance = NewAnimInstanceClass;
    if (AnimInstance)
    {
        AnimInstance->SetSkeletalComponent(this);
    }
}

void USkeletalMeshComponent::ForceRecomputePose()
{
    if (!SkeletalMesh) { return; } 

    // LocalSpace -> ComponentSpace 계산
    UpdateComponentSpaceTransforms();
    // ComponentSpace -> Final Skinning Matrices 계산
    UpdateFinalSkinningMatrices();
    UpdateSkinningMatrices(TempFinalSkinningMatrices, TempFinalSkinningNormalMatrices);
    PerformSkinning();
}

void USkeletalMeshComponent::UpdateComponentSpaceTransforms()
{
    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    const int32 NumBones = Skeleton.Bones.Num();

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FTransform& LocalTransform = CurrentLocalSpacePose[BoneIndex];
        const int32 ParentIndex = Skeleton.Bones[BoneIndex].ParentIndex;

        if (ParentIndex == -1) // 루트 본
        {
            CurrentComponentSpacePose[BoneIndex] = LocalTransform;
        }
        else // 자식 본
        {
            const FTransform& ParentComponentTransform = CurrentComponentSpacePose[ParentIndex];
            CurrentComponentSpacePose[BoneIndex] = ParentComponentTransform.GetWorldTransform(LocalTransform);
        }
    }
}

void USkeletalMeshComponent::UpdateFinalSkinningMatrices()
{
    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    const int32 NumBones = Skeleton.Bones.Num();

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FMatrix& InvBindPose = Skeleton.Bones[BoneIndex].InverseBindPose;
        const FMatrix ComponentPoseMatrix = CurrentComponentSpacePose[BoneIndex].ToMatrix();

        TempFinalSkinningMatrices[BoneIndex] = InvBindPose * ComponentPoseMatrix;
        TempFinalSkinningNormalMatrices[BoneIndex] = TempFinalSkinningMatrices[BoneIndex].Inverse().Transpose();
    }
}

void USkeletalMeshComponent::SetBoneLocalTransform(int32 BoneIndex, const FTransform& NewLocalTransform)
{
    if (CurrentLocalSpacePose.Num() > BoneIndex)
    {
        CurrentLocalSpacePose[BoneIndex] = NewLocalTransform;
        ForceRecomputePose();
    }
}

void USkeletalMeshComponent::SetBoneWorldTransform(int32 BoneIndex, const FTransform& NewWorldTransform)
{
    if (BoneIndex < 0 || BoneIndex >= CurrentLocalSpacePose.Num())
        return;

    const int32 ParentIndex = SkeletalMesh->GetSkeleton()->Bones[BoneIndex].ParentIndex;

    const FTransform& ParentWorldTransform = GetBoneWorldTransform(ParentIndex);
    FTransform DesiredLocal = ParentWorldTransform.GetRelativeTransform(NewWorldTransform);

    SetBoneLocalTransform(BoneIndex, DesiredLocal);
}


FTransform USkeletalMeshComponent::GetBoneLocalTransform(int32 BoneIndex) const
{
    if (CurrentLocalSpacePose.Num() > BoneIndex)
    {
        return CurrentLocalSpacePose[BoneIndex];
    }
    return FTransform();
}

FTransform USkeletalMeshComponent::GetBoneWorldTransform(int32 BoneIndex)
{
    if (CurrentLocalSpacePose.Num() > BoneIndex && BoneIndex >= 0)
    {
        // 뼈의 컴포넌트 공간 트랜스폼 * 컴포넌트의 월드 트랜스폼
        return GetWorldTransform().GetWorldTransform(CurrentComponentSpacePose[BoneIndex]);
    }
    return GetWorldTransform(); // 실패 시 컴포넌트 위치 반환
}

