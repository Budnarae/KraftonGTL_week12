#include "pch.h"
#include "AnimNode.h"

// ====================================
// FAnimStateTransition 구현
// ====================================

void FAnimStateTransition::CleanupDelegate()
{
    if (AssociatedRule && DelegateHandle != 0)
    {
        AssociatedRule->GetTransitionDelegate().Remove(DelegateHandle);
        DelegateHandle = 0;
        AssociatedRule = nullptr;
    }
}

FAnimStateTransition::~FAnimStateTransition()
{
    CleanupDelegate();
}

FAnimStateTransition::FAnimStateTransition(const FAnimStateTransition& Other)
    : SourceState(Other.SourceState)
    , TargetState(Other.TargetState)
    , Index(Other.Index)
    , CanEnterTransition(false)  // 새로 생성되는 Transition은 항상 false로 초기화
    , AssociatedRule(nullptr)  // Delegate는 복사하지 않음
    , DelegateHandle(0)
    , BlendTime(Other.BlendTime)
{
    // Delegate는 복사 후 재바인딩 필요
}

FAnimStateTransition& FAnimStateTransition::operator=(const FAnimStateTransition& Other)
{
    if (this != &Other)
    {
        // 기존 Delegate 정리
        CleanupDelegate();

        SourceState = Other.SourceState;
        TargetState = Other.TargetState;
        Index = Other.Index;
        CanEnterTransition = Other.CanEnterTransition;
        BlendTime = Other.BlendTime;;

        // Delegate는 복사하지 않음 (재바인딩 필요)
        AssociatedRule = nullptr;
        DelegateHandle = 0;
    }
    return *this;
}

FAnimState* FAnimNode_StateMachine::FindStateByName(const FName& StateName) const
{
    for (FAnimState* State : States)
    {
        if (State->Name == StateName)
            return State;
    }
    return nullptr;
}

FAnimNode_StateMachine::~FAnimNode_StateMachine()
{
    while (!States.empty())
    {
        DeleteState(States.back()->Name);
    }

    while (!Transitions.empty())
    {
        FAnimStateTransition* Transition = Transitions.back();
        delete Transition;
        Transitions.pop_back();
    }

    CurrentState = nullptr;
    CurrentTransition = nullptr;
}

void FAnimNode_StateMachine::Update(const FAnimationUpdateContext& Context)
{
    if (bIsInTransition)
    {
        if (!CurrentTransition) return;

        TransitionBlendNode.Update(Context);

        TransitionElapsed += Context.DeltaTime;
        float Alpha = (TransitionDuration > 0.f)
            ? FMath::Clamp(TransitionElapsed / TransitionDuration, 0.f, 1.f)
            : 1.f;

        TransitionBlendNode.Alpha = Alpha;

        if (Alpha >= 1.f)
        {
            // 전이 완료 → TargetState 정착
            if (CurrentTransition && CurrentTransition->TargetState)
            {
                CurrentState = CurrentTransition->TargetState;
            }

            bIsInTransition = false;
            CurrentTransition = nullptr;
            TransitionElapsed = 0.f;
            TransitionDuration = 0.f;
        }
    }
    else
    {
        if (!CurrentState || CurrentState->AnimSequenceNodes.empty())
            return;

        // 조건을 충족한 Transition이 있으면 State를 변환한다.
        for (FAnimStateTransition* Transition : Transitions)
        {
            if (!Transition) continue;

            if (Transition->CanEnterTransition &&
                Transition->SourceState == CurrentState)
            {
                // --- 전이 시작 설정 ---
                CurrentTransition = Transition;
                bIsInTransition = true;
                TransitionElapsed = 0.f;
                TransitionDuration = Transition->BlendTime;

                // Source/Target의 entry 시퀀스 노드
                FAnimNode_Sequence* SourceNode = nullptr;
                FAnimNode_Sequence* TargetNode = nullptr;

                if (CurrentState->AnimSequenceNodes.Num() > 0)
                    SourceNode = &CurrentState->AnimSequenceNodes[0];

                if (Transition->TargetState &&
                    Transition->TargetState->AnimSequenceNodes.Num() > 0)
                    TargetNode = &Transition->TargetState->AnimSequenceNodes[0];

                // 간단한 페이즈 맞추기
                if (SourceNode && TargetNode &&
                    SourceNode->Sequence && TargetNode->Sequence)
                {
                    const float SrcLen = SourceNode->Sequence->GetPlayLength();
                    const float DstLen = TargetNode->Sequence->GetPlayLength();

                    if (SrcLen > 0.f && DstLen > 0.f)
                    {
                        float Phase = SourceNode->CurrentTime / SrcLen;
                        Phase = FMath::Clamp(Phase, 0.f, 1.f);
                        TargetNode->CurrentTime = Phase * DstLen;
                    }
                }

                // 블렌드 노드 구성
                TransitionBlendNode.From = SourceNode;
                TransitionBlendNode.To = TargetNode;
                TransitionBlendNode.Alpha = 0.f;
                TransitionBlendNode.BlendTime = Transition->BlendTime;
                TransitionBlendNode.bIsBlending = true;

                return; // 이번 프레임은 전이 세팅까지만
            }

            // 전이 없다 -> 현재 상태의 그래프만 Update
            FAnimNode_Sequence& SeqNode = CurrentState->AnimSequenceNodes[0];
            SeqNode.Update(Context);
        }
    }
}
    
void FAnimNode_StateMachine::Evaluate(FPoseContext& Output)
{
    if (bIsInTransition && CurrentTransition)
    {
        TransitionBlendNode.Evaluate(Output);
    }
    else
    {
        if (!CurrentState || CurrentState->AnimSequenceNodes.empty())
            return;

        FAnimNode_Sequence* SequenceToEvaluate = &CurrentState->AnimSequenceNodes.front();
        SequenceToEvaluate->Evaluate(Output);
    }
}

FAnimState* FAnimNode_StateMachine::AddState(const FName& StateName)
{
    // 같은 이름을 가진 State가 있으면 거부
    for (const FAnimState* State : States)
    {
        if (State->Name == StateName)
        {
            UE_LOG("[FAnimNode_StateMachine::AddState] Warning : A state with the same name already exists in the Animation State Machine.");
            return nullptr;
        }
    }

    // 내부에서 동적 할당으로 State 생성
    FAnimState* State = new FAnimState();
    State->Name = StateName;
    State->Index = States.Num();
    States.push_back(State);

    // 비어있는 그래프였다면 첫 번째 State를 CurrentState로 설정
    if (!CurrentState)
    {
        CurrentState = State;
    }

    return State;
}

void FAnimNode_StateMachine::DeleteState(const FName& TargetName)
{
    // 삭제할 State 찾기
    FAnimState* TargetState = FindStateByName(TargetName);

    if (!TargetState)
    {
        UE_LOG("[FAnimNode_StateMachine::DeleteState] Warning : The state you are trying to delete does not exist in the state machine.");
        return;
    }

    // 해당 State를 참조하는 Transition들을 먼저 제거
    for (int32 i = Transitions.Num() - 1; i >= 0; i--)
    {
        FAnimStateTransition* Transition = Transitions[i];
        if (Transition->SourceState == TargetState ||
            Transition->TargetState == TargetState)
        {
             delete Transition;
            Transitions.erase(Transitions.begin() + i);
        }
    }

    // State 제거
    for (auto It = States.begin(); It != States.end(); ++It)
    {
        if (*It == TargetState)
        {
            delete* It;
            States.erase(It);
            break;
        }
    }

    // CurrentState가 삭제된 경우 nullptr로 설정
    if (CurrentState == TargetState)
    {
        CurrentState = States.Num() > 0 ? States[0] : nullptr;
    }
}

FAnimStateTransition* FAnimNode_StateMachine::AddTransition
(
    const FName& SourceName,
    const FName& TargetName
)
{
    // 입력된 이름과 일치하는 State 찾기
    FAnimState* SourceState = FindStateByName(SourceName);
    FAnimState* TargetState = FindStateByName(TargetName);

    if (!SourceState || !TargetState)
    {
        UE_LOG("[FAnimNode_StateMachine::AddTransition] Warning : The target state you are trying to connect with this transition does not exist in the state machine.");
        return nullptr;
    }

    // 이미 같은 노드에 대한 연결이 있으면 거부
    for (const FAnimStateTransition* Transition : Transitions)
    {
        if (!Transition) continue;
        if (Transition->SourceState == SourceState &&
            Transition->TargetState == TargetState)
        {
            UE_LOG("[FAnimNode_StateMachine::AddTransition] Warning : A transition to the same target state already exists in the state machine.");
            return nullptr;
        }
    }

    // 내부에서 동적 할당으로 Transition 생성
    FAnimStateTransition* Transition = new FAnimStateTransition();
    Transition->SourceState = SourceState;
    Transition->TargetState = TargetState;
    Transition->Index = Transitions.Num();
    Transitions.push_back(Transition);

    return Transition;
}

void FAnimNode_StateMachine::DeleteTransition(const FName& SourceName, const FName& TargetName)
{
    // 입력된 이름과 일치하는 State 찾기
    FAnimState* SourceState = FindStateByName(SourceName);
    FAnimState* TargetState = FindStateByName(TargetName);

    if (!SourceState || !TargetState)
    {
        UE_LOG("[FAnimNode_StateMachine::DeleteTransition] Warning : The transition you are trying to delete does not exist in the state machine.");
        return;
    }

    for (auto It = Transitions.begin(); It != Transitions.end(); It++)
    {
        if ((*It)->SourceState == SourceState &&
            (*It)->TargetState == TargetState)
        {
            delete* It;
            Transitions.erase(It);
            return;
        }
    }
    UE_LOG("[FAnimNode_StateMachine::DeleteTransition] Warning : The transition you are trying to delete does not exist in the state machine.");
}