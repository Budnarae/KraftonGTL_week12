#include "pch.h"
#include "TempCharacterAnimInstance.h"
#include "AnimNode.h"
#include "AnimationSequence.h"
#include "SkeletalMeshComponent.h"
#include "SkeletalMesh.h"
#include "ResourceManager.h"

IMPLEMENT_CLASS(UTempCharacterAnimInstance)

UTempCharacterAnimInstance::~UTempCharacterAnimInstance()
{
    // AnimNode들은 직접 관리하므로 삭제
    delete IdleSequenceNode;
    delete WalkSequenceNode;
    delete RunSequenceNode;
    delete MoveBlendSpaceNode;
}

void UTempCharacterAnimInstance::SetSpeed(float InSpeed)
{
    CurrentSpeed = InSpeed;

    // BlendSpace에 속도값 전달
    if (MoveBlendSpaceNode)
    {
        MoveBlendSpaceNode->SetBlendInput(CurrentSpeed);
    }
}

void UTempCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    // 첫 업데이트 시 초기화
    if (!bAnimationInitialized && OwnerSkeletalComp)
    {
        InitializeAnimationStateMachine();
    }

    // BlendSpace 업데이트
    if (MoveBlendSpaceNode)
    {
        FAnimationUpdateContext Context;
        Context.DeltaTime = DeltaSeconds;
        MoveBlendSpaceNode->Update(Context);
    }
}

void UTempCharacterAnimInstance::EvaluateAnimation()
{
    if (!OwnerSkeletalComp || !MoveBlendSpaceNode)
        return;

    USkeletalMesh* SkeletalMesh = OwnerSkeletalComp->GetSkeletalMesh();
    if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData())
        return;

    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    FPoseContext Out(&Skeleton);

    // BlendSpace에서 포즈 평가
    MoveBlendSpaceNode->Evaluate(Out);

    CurrentPose = Out;

    // 결과를 SkeletalMeshComponent에 적용
    if (Out.EvaluatedPoses.Num() > 0)
    {
        TArray<FTransform>& LocalPose = OwnerSkeletalComp->GetLocalSpacePose();
        LocalPose = Out.EvaluatedPoses;
        OwnerSkeletalComp->ForceRecomputePose();
    }
}

void UTempCharacterAnimInstance::InitializeAnimationStateMachine()
{
    if (bAnimationInitialized)
        return;

    // 애니메이션 로드
    UAnimationSequence* IdleAnimation = UResourceManager::GetInstance().Load<UAnimationSequence>("Standard Idle_mixamo.com");
    UAnimationSequence* WalkAnimation = UResourceManager::GetInstance().Load<UAnimationSequence>("Standard Walk_mixamo.com");
    UAnimationSequence* RunAnimation = UResourceManager::GetInstance().Load<UAnimationSequence>("Standard Run_mixamo.com");

    // 로드 검증
    if (!IdleAnimation || !WalkAnimation || !RunAnimation)
    {
        UE_LOG("[UTempCharacterAnimInstance] Failed to load animations");
        return;
    }

    if (!IdleAnimation->GetDataModel() || !WalkAnimation->GetDataModel() || !RunAnimation->GetDataModel())
    {
        UE_LOG("[UTempCharacterAnimInstance] Animation data models not found");
        return;
    }

    // Skeleton 정보 설정
    if (!OwnerSkeletalComp || !OwnerSkeletalComp->GetSkeletalMesh())
        return;

    USkeletalMesh* SkeletalMesh = OwnerSkeletalComp->GetSkeletalMesh();
    if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData())
        return;

    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;

    // 각 애니메이션에 Skeleton 설정 및 루핑 활성화
    IdleAnimation->SetSkeleton(Skeleton);
    IdleAnimation->SetLooping(true);

    WalkAnimation->SetSkeleton(Skeleton);
    WalkAnimation->SetLooping(true);

    RunAnimation->SetSkeleton(Skeleton);
    RunAnimation->SetLooping(true);

    // 시퀀스 노드 생성
    IdleSequenceNode = new FAnimNode_Sequence();
    IdleSequenceNode->SetSequence(IdleAnimation, true);

    WalkSequenceNode = new FAnimNode_Sequence();
    WalkSequenceNode->SetSequence(WalkAnimation, true);

    RunSequenceNode = new FAnimNode_Sequence();
    RunSequenceNode->SetSequence(RunAnimation, true);

    // BlendSpace1D 생성
    MoveBlendSpaceNode = new FAnimNode_BlendSpace1D();
    MoveBlendSpaceNode->MinimumPosition = 0.0f;
    MoveBlendSpaceNode->MaximumPosition = 600.0f;

    // 샘플 추가: 속도에 따른 애니메이션 블렌딩
    // 0: Idle, 200: Walk, 500: Run
    MoveBlendSpaceNode->AddSample(IdleSequenceNode, 0.0f);
    MoveBlendSpaceNode->AddSample(WalkSequenceNode, 200.0f);
    MoveBlendSpaceNode->AddSample(RunSequenceNode, 500.0f);

    MoveBlendSpaceNode->bIsTimeSynchronized = true;
    MoveBlendSpaceNode->SetBlendInput(CurrentSpeed);

    // AnimationMode를 AnimationBlueprint로 설정
    OwnerSkeletalComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);

    bAnimationInitialized = true;

    UE_LOG("[UTempCharacterAnimInstance] Animation initialized successfully");
}
