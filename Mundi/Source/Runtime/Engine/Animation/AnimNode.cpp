#include "pch.h"
#include "AnimNode.h"
#include "AnimNodeTransitionRule.h"

FAnimState* FAnimNode_StateMachine::FindStateByName(const FName& StateName) const
{
    for (FAnimState* State : States)
    {
        if (State->Name == StateName)
            return State;
    }
    return nullptr;
}

void FAnimNode_StateMachine::Update(const FAnimationUpdateContext& Context)
{
    if (bIsInTransition)
    {
        if (!CurrentTransition) return;

        CurrentTransition->Update(Context);

        // Transition 중에도 Target State 애니메이션 업데이트 (블렌딩 전까지 임시)
        if (CurrentTransition->TargetState &&
            !CurrentTransition->TargetState->AnimSequenceNodes.empty())
        {
            FAnimNode_Sequence* SequenceToUpdate = &CurrentTransition->TargetState->AnimSequenceNodes.front();
            if (SequenceToUpdate)
            {
                SequenceToUpdate->Update(Context);
            }
        }

        // Transition 완료 처리
        if (!CurrentTransition->bIsBlending)
        {
            CurrentState = CurrentTransition->TargetState;
            bIsInTransition = false;
            CurrentTransition->CanEnterTransition = false;
            CurrentTransition = nullptr;
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
                CurrentTransition = Transition;
                bIsInTransition = true;
                Transition->StartBlending();
                return;
            }
        }

        // 블렌드 구현 이전까지 State의 첫번째 Sequence를 우선하여 적용합니다.
        FAnimNode_Sequence* SequenceToUpdate = &CurrentState->AnimSequenceNodes.front();
        if (SequenceToUpdate)
        {
            SequenceToUpdate->Update(Context);
        }
    }
}

void FAnimNode_StateMachine::Evaluate(FPoseContext& Output)
{
    if (bIsInTransition)
    {
        // 블렌딩 구현 전까지 임시로 Target State를 평가
        if (CurrentTransition->TargetState &&
            !CurrentTransition->TargetState->AnimSequenceNodes.empty())
        {
            FAnimNode_Sequence* SequenceToEvaluate = &CurrentTransition->TargetState->AnimSequenceNodes.front();
            SequenceToEvaluate->Evaluate(Output);
        }
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
            Transition->CleanupDelegate();
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

FAnimStateTransition* FAnimNode_StateMachine::AddTransition
(
    const FName& SourceName,
    const FName& TargetName,
    UAnimNodeTransitionRule* TransitionRule
)
{
    // 기본 Transition 생성
    FAnimStateTransition* Transition = AddTransition(SourceName, TargetName);

    if (!Transition)
        return nullptr;

    if (!TransitionRule)
    {
        UE_LOG("[FAnimNode_StateMachine::AddTransition] Warning : TransitionRule is nullptr.");
        return Transition;
    }

    // Rule 연결 및 Delegate 바인딩
    Transition->AssociatedRule = TransitionRule;
    Transition->DelegateHandle = TransitionRule->GetTransitionDelegate().AddDynamic(
        Transition, &FAnimStateTransition::TriggerTransition);

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
            (*It)->CleanupDelegate();
            delete* It;
            Transitions.erase(It);
            return;
        }
    }
    UE_LOG("[FAnimNode_StateMachine::DeleteTransition] Warning : The transition you are trying to delete does not exist in the state machine.");
}
