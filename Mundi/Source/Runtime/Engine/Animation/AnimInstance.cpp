#include "pch.h"
#include "AnimInstance.h"
#include "SkeletalMeshComponent.h"
#include "AnimationAsset.h"
#include "AnimationSequence.h"
#include "AnimNode.h"
#include "ResourceManager.h"
#include "FloatComparisonRule.h"
#include "LuaScriptComponent.h"
#include "AnimNotify/AnimNotify.h"

IMPLEMENT_CLASS(UAnimInstance)
UAnimInstance::~UAnimInstance()
{
    // OwnerSkeletalComp는 이 클래스가 소유한 것이 아니라 참조만 하므로 delete하지 않음
    // (SkeletalMeshComponent가 AnimInstance를 소유하고 있음)
    OwnerSkeletalComp = nullptr;

    // C++ ASM 사용 시 (현재 주석 처리 - Lua ASM 사용)
    // if (RootNode)
    // {
    //     RootNode = nullptr;
    // }
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
    NativeUpdateAnimation(DeltaTime);

    // C++ ASM 사용 시 (현재 주석 처리 - Lua ASM 사용)
    // FAnimationUpdateContext Context;
    // Context.DeltaTime = DeltaTime;
    // if (RootNode)
    // {
    //     RootNode->Update(Context);
    // }

    // Anim Graph Evaluate
    EvaluateAnimation();

    // AnimNotify 처리
    PostUpdateAnimation();
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

    // Lua ASM 사용: AnimUpdate() 호출하여 현재 재생 중인 AnimSequence 가져오기
    // OwnerSkeletalComp의 Owner Actor의 Lua 스크립트에서 AnimUpdate() 함수 호출

    // LastAnimationTime 갱신 (현재 시간을 저장)
    LastAnimationTime = CurrentAnimationTime;

    if (OwnerSkeletalComp)
    {
        AActor* OwnerActor = OwnerSkeletalComp->GetOwner();
        if (OwnerActor)
        {
            ULuaScriptComponent* ScriptComp = static_cast<ULuaScriptComponent*>(
                OwnerActor->GetComponent(ULuaScriptComponent::StaticClass())
            );
            if (ScriptComp)
            {
                // AnimUpdate를 호출하고 현재 재생 중인 AnimNode_Sequence를 반환받음
                FAnimNode_Sequence* CurrentNode = nullptr;
                if (ScriptComp->CallFunctionWithReturn("AnimUpdate", CurrentNode, DeltaSeconds))
                {
                    if (CurrentNode)
                    {
                        // Node에서 현재 시간과 Sequence 가져옴
                        CurrentAnimationTime = CurrentNode->CurrentTime;
                        CurrentAnimation = CurrentNode->Sequence;
                    }
                }
            }
        }
    }
}

void UAnimInstance::PostUpdateAnimation()
{
    // 현재 재생 중인 애니메이션이 없으면 조기 반환
    if (!CurrentAnimation)
        return;

    // 현재 애니메이션의 AnimNotify 목록 가져오기
    const TArray<UAnimNotify*>& AnimNotifies = CurrentAnimation->GetAnimNotifies();

    // LastAnimationTime과 CurrentAnimationTime 사이에 있는 AnimNotify 실행
    for (UAnimNotify* Notify : AnimNotifies)
    {
        if (!Notify)
            continue;

        float NotifyTime = Notify->GetTimeToNotify();

        // 시간 범위 체크: LastAnimationTime < NotifyTime <= CurrentAnimationTime
        // 루프 애니메이션 고려
        bool bShouldTrigger = false;

        if (LastAnimationTime <= CurrentAnimationTime)
        {
            // 일반 경우: 시간이 순방향으로 진행
            if (NotifyTime > LastAnimationTime && NotifyTime <= CurrentAnimationTime)
            {
                bShouldTrigger = true;
            }
        }
        else
        {
            // 루프 경우: 애니메이션이 끝에서 처음으로 돌아감
            // LastTime이 CurrentTime보다 크다는 것은 루프가 발생했다는 의미
            if (NotifyTime > LastAnimationTime || NotifyTime <= CurrentAnimationTime)
            {
                bShouldTrigger = true;
            }
        }

        // Notify 실행 (조건이 맞으면 실행)
        if (bShouldTrigger)
        {
            Notify->Notify();
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
        OwnerSkeletalComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
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

    // 4) RootNode를 StateMachine으로 고정
    RootNode = &ASM;
}
*/