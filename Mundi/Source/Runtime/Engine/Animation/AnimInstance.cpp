#include "pch.h"
#include "AnimInstance.h"
#include "SkeletalMeshComponent.h"
#include "LuaScriptComponent.h"

IMPLEMENT_CLASS(UAnimInstance)
UAnimInstance::~UAnimInstance()
{
    // OwnerSkeletalComp는 이 클래스가 소유한 것이 아니라 참조만 하므로 delete하지 않음
    // (SkeletalMeshComponent가 AnimInstance를 소유하고 있음)
    OwnerSkeletalComp = nullptr;
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

    // C++ ASM 사용 시 (현재 주석 처리 - Lua ASM 사용)
    // InitializeAnimationStateMachine();
}

void UAnimInstance::UpdateAnimation(float DeltaTime)
{
    if (!OwnerSkeletalComp)
        return;

    // 변수 업데이트
    NativeUpdateAnimation(DeltaTime * GlobalSpeed);

    // C++ ASM 사용 시 (현재 주석 처리 - Lua ASM 사용)
    // FAnimationUpdateContext Context;
    // Context.DeltaTime = DeltaTime;
    // if (RootNode)
    // {
    //     RootNode->Update(Context);
    // }

    // Anim Graph Evaluate
    EvaluateAnimation();
}


void UAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    // C++ ASM 사용 시 (현재 주석 처리 - Lua ASM 사용)
    // TransitionTimer += DeltaSeconds;
    // for (UAnimNodeTransitionRule* Rule : TransitionRules)
    // {
    //     if (Rule)
    //     {
    //         UFloatComparisonRule* FloatRule = dynamic_cast<UFloatComparisonRule*>(Rule);
    //         if (FloatRule)
    //         {
    //             FloatRule->SetComparisonValue(TransitionTimer);
    //         }
    //         Rule->Evaluate();
    //     }
    // }
    // FAnimState* CurrentState = ASM.GetCurrentState();
    // if (CurrentState != PreviousState)
    // {
    //     TransitionTimer = 0.0f;
    //     PreviousState = CurrentState;
    //     TArray<FAnimStateTransition*>& Transitions = ASM.GetTransitions();
    //     for (FAnimStateTransition* Transition : Transitions)
    //     {
    //         if (Transition)
    //         {
    //             Transition->CanEnterTransition = false;
    //         }
    //     }
    // }

    //CurrentMoveSpeed += DeltaSeconds * 20.0f;
    //if (CurrentMoveSpeed > 600.0f) { CurrentMoveSpeed = 0.0f; }

    //if (MoveBlendSpaceNode)
    //{
    //    MoveBlendSpaceNode->SetBlendInput(CurrentMoveSpeed);
    //}
    
    AActor* OwnerActor = OwnerSkeletalComp ? OwnerSkeletalComp->GetOwner() : nullptr;
    if (OwnerActor)
    {
        ULuaScriptComponent* ScriptComp = static_cast<ULuaScriptComponent*>(
            OwnerActor->GetComponent(ULuaScriptComponent::StaticClass())
        );
        if (ScriptComp)
        {
            ScriptComp->CallFunction("AnimUpdate", DeltaSeconds);
        }
    }
}

void UAnimInstance::EvaluateAnimation()
{
    if (!OwnerSkeletalComp) return;

    // C++ ASM 사용 시 (현재 주석 처리 - Lua ASM 사용)
    // if (!RootNode) return;
    // USkeletalMesh* SkeletalMesh = OwnerSkeletalComp->GetSkeletalMesh();
    // if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData()) return;
    // const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    // FPoseContext Out(&Skeleton);
    // RootNode->Evaluate(Out);
    // CurrentPose = Out;
    // if (Out.EvaluatedPoses.Num() > 0)
    // {
    //     TArray<FTransform>& LocalPose = OwnerSkeletalComp->GetLocalSpacePose();
    //     LocalPose = Out.EvaluatedPoses;
    //     OwnerSkeletalComp->ForceRecomputePose();
    // }

    // Lua ASM 사용: AnimEvaluate() 호출
    // OwnerSkeletalComp의 Owner Actor의 Lua 스크립트에서 AnimEvaluate() 함수 호출
    AActor* OwnerActor = OwnerSkeletalComp->GetOwner();
    if (OwnerActor)
    {
        ULuaScriptComponent* ScriptComp = static_cast<ULuaScriptComponent*>(
            OwnerActor->GetComponent(ULuaScriptComponent::StaticClass())
        );
        if (ScriptComp)
        {
            USkeletalMesh* SkeletalMesh = OwnerSkeletalComp->GetSkeletalMesh();
            if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshData()) return;

            const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
            FPoseContext Out(&Skeleton);

            // Lua 함수에 FPoseContext 전달
            ScriptComp->CallFunction("AnimEvaluate", &Out);

            CurrentPose = Out;

            if (Out.EvaluatedPoses.Num() > 0)
            {
                TArray<FTransform>& LocalPose = OwnerSkeletalComp->GetLocalSpacePose();
                LocalPose = Out.EvaluatedPoses;
                OwnerSkeletalComp->ForceRecomputePose();
            }
        }
    }
}

// C++ ASM 관련 함수 (주석 처리 - Lua ASM 사용)
/*
void UAnimInstance::AddTransitionRule(UAnimNodeTransitionRule* InRule)
{
    if (!InRule)
    {
        UE_LOG("[UAnimInstance::AddTransitionRule] Warning : Trying to add null rule.");
        return;
    }

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
    UAnimationSequence* RunAnimation = UResourceManager::GetInstance().Load<UAnimationSequence>("Standard Run_mixamo.com");
    UAnimationSequence* WalkAnimation = UResourceManager::GetInstance().Load<UAnimationSequence>("Standard Walk_mixamo.com");
    UAnimationSequence* IdleAnimation = UResourceManager::GetInstance().Load<UAnimationSequence>("James_mixamo.com");

    // 로드 검증
    if (!RunAnimation || !WalkAnimation || !IdleAnimation)
        return;

    if (!RunAnimation->GetDataModel() || !WalkAnimation->GetDataModel() || !IdleAnimation->GetDataModel())
        return;

    // Skeleton 정보 설정 (SkeletalMeshComponent로부터)
    if (OwnerSkeletalComp && OwnerSkeletalComp->GetSkeletalMesh())
    {
        USkeletalMesh* SkeletalMesh = OwnerSkeletalComp->GetSkeletalMesh();
        if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshData())
        {
            const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;

            // 각 애니메이션에 Skeleton 설정 및 루핑 활성화
            RunAnimation->SetSkeleton(Skeleton);
            RunAnimation->SetLooping(true);

            WalkAnimation->SetSkeleton(Skeleton);
            WalkAnimation->SetLooping(true);

            IdleAnimation->SetSkeleton(Skeleton);
            IdleAnimation->SetLooping(true);
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

    IdleSequenceNode = NewNode<FAnimNode_Sequence>();
    IdleSequenceNode->SetSequence(IdleAnimation, true);

    WalkSequenceNode = NewNode<FAnimNode_Sequence>();
    WalkSequenceNode->SetSequence(WalkAnimation, true);

    RunSequenceNode = NewNode<FAnimNode_Sequence>();
    RunSequenceNode->SetSequence(RunAnimation, true);

    MoveBlendSpaceNode = NewNode<FAnimNode_BlendSpace1D>();
    MoveBlendSpaceNode->MinimumPosition = 0.0f;
    MoveBlendSpaceNode->MaximumPosition = 600.0f; // 예: 이동 속도 최댓값

    MoveBlendSpaceNode->AddSample(IdleSequenceNode, 0.0f);
    MoveBlendSpaceNode->AddSample(WalkSequenceNode, 200.0f);
    MoveBlendSpaceNode->AddSample(RunSequenceNode, 500.0f);

    MoveBlendSpaceNode->IsTimeSynchronized = true;
    MoveBlendSpaceNode->SetBlendInput(CurrentMoveSpeed);

    FAnimState* LocomotionState = ASM.AddState("Locomotion");
    if (LocomotionState)
    {
        LocomotionState->SetEntryNode(MoveBlendSpaceNode);
    }

    RootNode = &ASM;

    if (OwnerSkeletalComp)
    {
        OwnerSkeletalComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    }
    //// State 생성 및 애니메이션 추가 (새로운 API 사용)
    //// AddState가 내부에서 동적 할당하여 State 포인터를 반환
    //FAnimState* StateA = ASM.AddState("StateA");
    //if (StateA)
    //{
    //    FAnimNode_Sequence* SequenceNode = StateA->CreateNode<FAnimNode_Sequence>();
    //    SequenceNode->SetSequence(AnimA);
    //    SequenceNode->SetLooping(true);
    //    StateA->SetEntryNode(SequenceNode);
    //}

    //FAnimState* StateB = ASM.AddState("StateB");
    //if (StateB)
    //{
    //    FAnimNode_Sequence* SequenceNode = StateB->CreateNode<FAnimNode_Sequence>();
    //    SequenceNode->SetSequence(AnimB);
    //    SequenceNode->SetLooping(true);
    //    StateB->SetEntryNode(SequenceNode);
    //}

    //FAnimState* StateC = ASM.AddState("StateC");
    //if (StateC)
    //{
    //    FAnimNode_Sequence* SequenceNode = StateC->CreateNode<FAnimNode_Sequence>();
    //    SequenceNode->SetSequence(AnimC);
    //    SequenceNode->SetLooping(true);
    //    StateC->SetEntryNode(SequenceNode);
    //}

    //// AnimationMode를 AnimationBlueprint로 설정
    //if (OwnerSkeletalComp)
    //{
    //    OwnerSkeletalComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    //}

    //// Transition Rule 생성 및 추가 (10초마다 전환)
    //UFloatComparisonRule* RuleAtoB = NewObject<UFloatComparisonRule>();
    //RuleAtoB->SetRuleName("RuleAtoB");
    //RuleAtoB->SetComparisonOperator(UAnimNodeTransitionRule::ComparisonOperator::GreaterThanOrEqualTo);
    //RuleAtoB->SetBaseValue(3.0f);  // 10초 기준
    //RuleAtoB->SetComparisonValue(0.0f);  // 매 프레임 업데이트됨
    //AddTransitionRule(RuleAtoB);

    //UFloatComparisonRule* RuleBtoC = NewObject<UFloatComparisonRule>();
    //RuleBtoC->SetRuleName("RuleBtoC");
    //RuleBtoC->SetComparisonOperator(UAnimNodeTransitionRule::ComparisonOperator::GreaterThanOrEqualTo);
    //RuleBtoC->SetBaseValue(3.0f);  // 10초 기준
    //RuleBtoC->SetComparisonValue(0.0f);  // 매 프레임 업데이트됨
    //AddTransitionRule(RuleBtoC);

    //UFloatComparisonRule* RuleCtoA = NewObject<UFloatComparisonRule>();
    //RuleCtoA->SetRuleName("RuleCtoA");
    //RuleCtoA->SetComparisonOperator(UAnimNodeTransitionRule::ComparisonOperator::GreaterThanOrEqualTo);
    //RuleCtoA->SetBaseValue(3.0f);  // 10초 기준
    //RuleCtoA->SetComparisonValue(0.0f);  // 매 프레임 업데이트됨
    //AddTransitionRule(RuleCtoA);

    //// Transition 생성 및 연결 (새로운 API 사용)
    //// AddTransition이 내부에서 동적 할당, Rule 연결, Delegate 바인딩을 모두 처리
    //FAnimStateTransition* TransitionAtoB = ASM.AddTransition("StateA", "StateB", RuleAtoB);
    //if (TransitionAtoB)
    //{
    //    TransitionAtoB->SetBlendTime(0.3f);
    //}

    //FAnimStateTransition* TransitionBtoC = ASM.AddTransition("StateB", "StateC", RuleBtoC);
    //if (TransitionBtoC)
    //{
    //    TransitionBtoC->SetBlendTime(0.3f);
    //}

    //FAnimStateTransition* TransitionCtoA = ASM.AddTransition("StateC", "StateA", RuleCtoA);
    //if (TransitionCtoA)
    //{
    //    TransitionCtoA->SetBlendTime(0.3f);
    //}

    //// 4) RootNode를 StateMachine으로 고정
    //RootNode = &ASM;
}
*/
