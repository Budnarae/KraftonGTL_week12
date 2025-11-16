#include "pch.h"
#include "AnimInstance.h"
#include "SkeletalMeshComponent.h"
#include "AnimationAsset.h"
#include "AnimBlend.h"
#include "AnimationSequence.h"
#include "ResourceManager.h"
#include "FloatComparisonRule.h"

IMPLEMENT_CLASS(UAnimInstance)
UAnimInstance::~UAnimInstance()
{
    if (TestSeqA)
    {
        delete TestSeqA;
        TestSeqA = nullptr;
    }
    if (TestSeqB)
    {
        delete TestSeqB;
        TestSeqB = nullptr;
    }
    if (OwnerSkeletalComp)
    {
        delete OwnerSkeletalComp;
        OwnerSkeletalComp = nullptr;
    }
    if (CurrentAnimation)
    {
        delete CurrentAnimation;
        CurrentAnimation = nullptr;
    }
}

void UAnimInstance::SetSkeletalComponent(USkeletalMeshComponent* InSkeletalMeshComponent)
{
    OwnerSkeletalComp = InSkeletalMeshComponent;

    // 최초 설정 시 초기화
    if (OwnerSkeletalComp && !bIsInitialized)
    {
        Initialize();
    }
}

void UAnimInstance::Initialize()
{
    if (bIsInitialized)
        return;

    bIsInitialized = true;

    // Animation State Machine 초기화
    InitializeAnimationStateMachine();
}

void UAnimInstance::UpdateAnimation(float DeltaTime)
{
    if (!OwnerSkeletalComp)
        return;

    // ====================================
    // Animation State Machine 기반 업데이트
    // ====================================
    NativeUpdateAnimation(DeltaTime);
    EvaluateAnimation();

    // 평가된 포즈를 SkeletalMeshComponent에 적용
    if (CurrentPose.EvaluatedPoses.Num() > 0)
    {
        TArray<FTransform>& LocalPose = OwnerSkeletalComp->GetLocalSpacePose();
        LocalPose = CurrentPose.EvaluatedPoses;
        OwnerSkeletalComp->ForceRecomputePose();
    }
}


void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    // 타이머 업데이트
    TransitionTimer += DeltaSeconds;

    // 매 프레임 모든 Rule에 현재 타이머 값 전달 및 평가
    for (UAnimNodeTransitionRule* Rule : TransitionRules)
    {
        if (Rule)
        {
            // FloatComparisonRule이라면 현재 시간 값을 전달
            UFloatComparisonRule* FloatRule = dynamic_cast<UFloatComparisonRule*>(Rule);
            if (FloatRule)
            {
                FloatRule->SetComparisonValue(TransitionTimer);
            }

            Rule->Evaluate();
        }
    }

    // State Machine 업데이트
    FAnimationUpdateContext Context;
    Context.DeltaTime = DeltaSeconds;
    ASM.Update(Context);

    // 상태가 변경되었는지 확인하여 타이머 리셋
    FAnimState* CurrentState = ASM.GetCurrentState();
    if (CurrentState != PreviousState)
    {
        TransitionTimer = 0.0f;
        PreviousState = CurrentState;

        // State가 변경되면 모든 Transition의 CanEnterTransition을 리셋
        // 이전 State에서 활성화된 Transition들이 새로운 State에서도 활성화되는 것을 방지
        TArray<FAnimStateTransition*>& Transitions = ASM.GetTransitions();
        for (FAnimStateTransition* Transition : Transitions)
        {
            if (Transition)
            {
                Transition->CanEnterTransition = false;
            }
        }
    }
}

void UAnimInstance::EvaluateAnimation()
{
    ASM.Evaluate(CurrentPose);
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
    // 애니메이션 로드
    UAnimationSequence* AnimA = UResourceManager::GetInstance().Load<UAnimationSequence>("Standard Run_mixamo.com");
    UAnimationSequence* AnimB = UResourceManager::GetInstance().Load<UAnimationSequence>("Standard Walk_mixamo.com");
    UAnimationSequence* AnimC = UResourceManager::GetInstance().Load<UAnimationSequence>("James_mixamo.com");

    // 로드 검증
    if (!AnimA || !AnimB || !AnimC)
        return;

    if (!AnimA->GetDataModel() || !AnimB->GetDataModel() || !AnimC->GetDataModel())
        return;

    // Skeleton 정보 설정 (SkeletalMeshComponent로부터)
    if (OwnerSkeletalComp && OwnerSkeletalComp->GetSkeletalMesh())
    {
        USkeletalMesh* SkeletalMesh = OwnerSkeletalComp->GetSkeletalMesh();
        if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshData())
        {
            const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;

            // 각 애니메이션에 Skeleton 설정 및 루핑 활성화
            AnimA->SetSkeleton(Skeleton);
            AnimA->SetLooping(true);

            AnimB->SetSkeleton(Skeleton);
            AnimB->SetLooping(true);

            AnimC->SetSkeleton(Skeleton);
            AnimC->SetLooping(true);
        }
        else
        {
            return;
        }
    }
    else
    {
        return;
    }

    // State 생성 및 애니메이션 추가 (새로운 API 사용)
    // AddState가 내부에서 동적 할당하여 State 포인터를 반환
    FAnimState* StateA = ASM.AddState("StateA");
    if (StateA)
    {
        StateA->AddAnimSequence(AnimA);
    }

    FAnimState* StateB = ASM.AddState("StateB");
    if (StateB)
    {
        StateB->AddAnimSequence(AnimB);
    }

    FAnimState* StateC = ASM.AddState("StateC");
    if (StateC)
    {
        StateC->AddAnimSequence(AnimC);
    }

    // AnimationMode를 AnimationBlueprint로 설정
    if (OwnerSkeletalComp)
    {
        OwnerSkeletalComp->SetAnimationMode(USkeletalMeshComponent::EAnimationMode::AnimationBlueprint);
    }

    // Transition Rule 생성 및 추가 (10초마다 전환)
    UFloatComparisonRule* RuleAtoB = NewObject<UFloatComparisonRule>();
    RuleAtoB->SetRuleName("RuleAtoB");
    RuleAtoB->SetComparisonOperator(UAnimNodeTransitionRule::ComparisonOperator::GreaterThanOrEqualTo);
    RuleAtoB->SetBaseValue(3.0f);  // 10초 기준
    RuleAtoB->SetComparisonValue(0.0f);  // 매 프레임 업데이트됨
    AddTransitionRule(RuleAtoB);

    UFloatComparisonRule* RuleBtoC = NewObject<UFloatComparisonRule>();
    RuleBtoC->SetRuleName("RuleBtoC");
    RuleBtoC->SetComparisonOperator(UAnimNodeTransitionRule::ComparisonOperator::GreaterThanOrEqualTo);
    RuleBtoC->SetBaseValue(3.0f);  // 10초 기준
    RuleBtoC->SetComparisonValue(0.0f);  // 매 프레임 업데이트됨
    AddTransitionRule(RuleBtoC);

    UFloatComparisonRule* RuleCtoA = NewObject<UFloatComparisonRule>();
    RuleCtoA->SetRuleName("RuleCtoA");
    RuleCtoA->SetComparisonOperator(UAnimNodeTransitionRule::ComparisonOperator::GreaterThanOrEqualTo);
    RuleCtoA->SetBaseValue(3.0f);  // 10초 기준
    RuleCtoA->SetComparisonValue(0.0f);  // 매 프레임 업데이트됨
    AddTransitionRule(RuleCtoA);

    // Transition 생성 및 연결 (새로운 API 사용)
    // AddTransition이 내부에서 동적 할당, Rule 연결, Delegate 바인딩을 모두 처리
    FAnimStateTransition* TransitionAtoB = ASM.AddTransition("StateA", "StateB", RuleAtoB);
    if (TransitionAtoB)
    {
        TransitionAtoB->SetBlendTime(0.3f);
    }

    FAnimStateTransition* TransitionBtoC = ASM.AddTransition("StateB", "StateC", RuleBtoC);
    if (TransitionBtoC)
    {
        TransitionBtoC->SetBlendTime(0.3f);
    }

    FAnimStateTransition* TransitionCtoA = ASM.AddTransition("StateC", "StateA", RuleCtoA);
    if (TransitionCtoA)
    {
        TransitionCtoA->SetBlendTime(0.3f);
    }
}

void UAnimInstance::PlayBlendedAnimation(UAnimationSequence& InSeqA, UAnimationSequence& InSeqB)
{
    TestSeqA = &InSeqA;
    TestSeqB = &InSeqB;
    IsBlending = true;
}

void UAnimInstance::UpdateBlendedAnimation(float DeltaTime, float Alpha)
{
    if (!OwnerSkeletalComp || !TestSeqA || !TestSeqB || !IsBlending)
    {
        return;
    }

    USkeletalMesh* SkeletalMesh = OwnerSkeletalComp->GetSkeletalMesh();
    if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData())
    {
        return;
    }

    CurTime += DeltaTime;

    // 2) 모든 본의 로컬 포즈 업데이트
    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    const int32 NumBones = Skeleton.Bones.Num();

    TArray<FTransform>& LocalPose = OwnerSkeletalComp->GetLocalSpacePose();
    LocalPose.SetNum(NumBones);

    FPoseContext TSeqA, TSeqB, Out;
    TestSeqA->EvaluatePose(CurTime, Skeleton, TSeqA);
    TestSeqB->EvaluatePose(CurTime, Skeleton, TSeqB);

    FAnimBlend::Blend(TSeqA, TSeqB, Alpha, Out);

    LocalPose = Out.EvaluatedPoses;

    // 포즈 재계산 => 스키닝
    OwnerSkeletalComp->ForceRecomputePose();
}