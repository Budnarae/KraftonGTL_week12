#include "pch.h"
#include "AnimInstance.h"
#include "SkeletalMeshComponent.h"
#include "AnimationAsset.h"

IMPLEMENT_CLASS(UAnimInstance)
void UAnimInstance::UpdateAnimation(float DeltaTime)
{
    // ====================================
    // [임시 코드 - 주석 처리됨]
    // 동료가 만든 직접 애니메이션 업데이트 코드
    // Animation State Machine으로 대체됨
    // ====================================
    /*
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
    */

    // ====================================
    // Animation State Machine 기반 업데이트로 대체
    // ====================================
    NativeUpdateAnimation(DeltaTime);
    EvaluateAnimation();

    // 평가된 포즈를 SkeletalMeshComponent에 적용
    if (OwnerSkeletalComp)
    {
        TArray<FTransform>& LocalPose = OwnerSkeletalComp->GetLocalSpacePose();
        LocalPose = CurrentPose.EvaluatedPoses;
        OwnerSkeletalComp->ForceRecomputePose();
    }
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
    // 10초마다 Transition Rule 트리거
    TransitionTimer += DeltaSeconds;
    if (TransitionTimer >= TransitionInterval)
    {
        TransitionTimer = 0.0f;

        // 모든 Rule을 순차적으로 평가
        // Rule의 CheckTransitionRule()을 호출하여 조건을 확인하고,
        // 조건이 맞으면 Evaluate()를 호출하여 Delegate를 발동
        for (UAnimNodeTransitionRule* Rule : TransitionRules)
        {
            if (Rule)
            {
                Rule->Evaluate();
            }
        }
    }

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

void UAnimInstance::AddTransitionRule(UAnimNodeTransitionRule* InRule)
{
    if (!InRule)
    {
        UE_LOG("[UAnimInstance::AddTransitionRule] Warning : Trying to add null rule.");
        return;
    }

    // 같은 이름의 Rule이 이미 있으면 거부
    for (UAnimNodeTransitionRule* Rule : TransitionRules)
    {
        if (Rule && Rule->GetRuleName() == InRule->GetRuleName())
        {
            UE_LOG("[UAnimInstance::AddTransitionRule] Warning : A rule with the same name already exists.");
            return;
        }
    }

    TransitionRules.Add(InRule);
}

void UAnimInstance::RemoveTransitionRule(const FName& RuleName)
{
    for (int32 i = TransitionRules.Num() - 1; i >= 0; i--)
    {
        if (TransitionRules[i] && TransitionRules[i]->GetRuleName() == RuleName)
        {
            TransitionRules.erase(TransitionRules.begin() + i);
            return;
        }
    }
    UE_LOG("[UAnimInstance::RemoveTransitionRule] Warning : The rule you are trying to remove does not exist.");
}

UAnimNodeTransitionRule* UAnimInstance::FindTransitionRuleByName(const FName& RuleName) const
{
    for (UAnimNodeTransitionRule* Rule : TransitionRules)
    {
        if (Rule && Rule->GetRuleName() == RuleName)
        {
            return Rule;
        }
    }
    return nullptr;
}

void UAnimInstance::InitializeAnimationStateMachine()
{
    // State 생성
    FAnimState StateA;
    StateA.Name = "StateA";
    // TODO: StateA.AnimSequences에 실제 애니메이션 추가 필요

    FAnimState StateB;
    StateB.Name = "StateB";
    // TODO: StateB.AnimSequences에 실제 애니메이션 추가 필요

    FAnimState StateC;
    StateC.Name = "StateC";
    // TODO: StateC.AnimSequences에 실제 애니메이션 추가 필요

    // State Machine에 State 추가
    ASM.AddState(StateA);
    ASM.AddState(StateB);
    ASM.AddState(StateC);

    // Transition Rule 생성 및 추가
    UAnimNodeTransitionRule* RuleAtoB = NewObject<UAnimNodeTransitionRule>();
    RuleAtoB->SetRuleName("RuleAtoB");
    AddTransitionRule(RuleAtoB);

    UAnimNodeTransitionRule* RuleBtoC = NewObject<UAnimNodeTransitionRule>();
    RuleBtoC->SetRuleName("RuleBtoC");
    AddTransitionRule(RuleBtoC);

    UAnimNodeTransitionRule* RuleCtoA = NewObject<UAnimNodeTransitionRule>();
    RuleCtoA->SetRuleName("RuleCtoA");
    AddTransitionRule(RuleCtoA);

    // Transition 생성 및 연결
    FAnimStateTransition TransitionAtoB;
    TransitionAtoB.BlendTime = 0.3f;
    FDelegateHandle HandleAtoB = RuleAtoB->GetTransitionDelegate().AddLambda([&TransitionAtoB]()
    {
        TransitionAtoB.TriggerTransition();
    });
    TransitionAtoB.DelegateHandle = HandleAtoB;

    FAnimStateTransition TransitionBtoC;
    TransitionBtoC.BlendTime = 0.3f;
    FDelegateHandle HandleBtoC = RuleBtoC->GetTransitionDelegate().AddLambda([&TransitionBtoC]()
    {
        TransitionBtoC.TriggerTransition();
    });
    TransitionBtoC.DelegateHandle = HandleBtoC;

    FAnimStateTransition TransitionCtoA;
    TransitionCtoA.BlendTime = 0.3f;
    FDelegateHandle HandleCtoA = RuleCtoA->GetTransitionDelegate().AddLambda([&TransitionCtoA]()
    {
        TransitionCtoA.TriggerTransition();
    });
    TransitionCtoA.DelegateHandle = HandleCtoA;

    // State Machine에 Transition 추가
    ASM.AddTransition("StateA", "StateB", TransitionAtoB);
    ASM.AddTransition("StateB", "StateC", TransitionBtoC);
    ASM.AddTransition("StateC", "StateA", TransitionCtoA);
}
