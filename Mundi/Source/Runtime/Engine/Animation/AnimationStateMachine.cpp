#include "pch.h"
#include "AnimationStateMachine.h"
#include "AnimationSequence.h"
#include "AnimNodeTransitionRule.h"

#include "Keyboard.h"

IMPLEMENT_CLASS(UAnimationStateMachine)

UAnimationStateMachine::UAnimationStateMachine()
{
}

UAnimationStateMachine::~UAnimationStateMachine()
{
    // States 메모리 해제
    for (FAnimState* State : States)
    {
        delete State;
    }
    States.clear();

    // Transitions 메모리 해제
    for (FAnimStateTransition* Transition : Transitions)
    {
        Transition->CleanupDelegate();
        delete Transition;
    }
    Transitions.clear();
}

FAnimState* UAnimationStateMachine::FindStateByName(const FName& StateName) const
{
    for (FAnimState* State : States)
    {
        if (State->Name == StateName)
            return State;
    }
    return nullptr;
}

void UAnimationStateMachine::Update(const FAnimationUpdateContext& Context)
{
    if (bIsInTransition)
    {
        CurrentTransition->Update(Context);

        // Transition 중에도 Target State 애니메이션 업데이트 (블렌딩 전까지 임시)
        if (CurrentTransition->TargetState &&
            !CurrentTransition->TargetState->AnimSequences.empty())
        {
            UAnimationSequence* SequenceToUpdate = CurrentTransition->TargetState->AnimSequences.front();
            SequenceToUpdate->Update(Context);
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
        if (!CurrentState || CurrentState->AnimSequences.empty())
            return;

        // 조건을 충족한 Transition이 있으면 State를 변환한다.
        for (FAnimStateTransition* Transition : Transitions)
        {
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
        UAnimationSequence* SequenceToUpdate = CurrentState->AnimSequences.front();
        SequenceToUpdate->Update(Context);
    }
}

void UAnimationStateMachine::Evaluate(FPoseContext& Output)
{
    if (bIsInTransition)
    {
        // 블렌딩 구현 전까지 임시로 Target State를 평가
        if (CurrentTransition->TargetState &&
            !CurrentTransition->TargetState->AnimSequences.empty())
        {
            UAnimationSequence* SequenceToEvaluate = CurrentTransition->TargetState->AnimSequences.front();
            SequenceToEvaluate->Evaluate(Output);
        }
    }
    else
    {
        if (!CurrentState || CurrentState->AnimSequences.empty())
            return;

        UAnimationSequence* SequenceToEvaluate = CurrentState->AnimSequences.front();
        SequenceToEvaluate->Evaluate(Output);
    }
}

FAnimState* UAnimationStateMachine::AddState(const FName& StateName)
{
    // 같은 이름을 가진 State가 있으면 거부
    for (const FAnimState* State : States)
    {
        if (State->Name == StateName)
        {
            UE_LOG("[UAnimationStateMachine::AddState] Warning : A state with the same name already exists in the Animation State Machine.");
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

void UAnimationStateMachine::DeleteState(const FName& TargetName)
{
    // 삭제할 State 찾기
    FAnimState* TargetState = FindStateByName(TargetName);

    if (!TargetState)
    {
        UE_LOG("[UAnimationStateMachine::DeleteState] Warning : The state you are trying to delete does not exist in the state machine.");
        return;
    }

    // 해당 State를 참조하는 Transition들을 먼저 제거
    for (int32 i = Transitions.Num() - 1; i >= 0; i--)
    {
        if (Transitions[i]->SourceState == TargetState ||
            Transitions[i]->TargetState == TargetState)
        {
            Transitions[i]->CleanupDelegate();
            delete Transitions[i];
            Transitions.erase(Transitions.begin() + i);
        }
    }

    // State 제거
    for (auto It = States.begin(); It != States.end(); ++It)
    {
        if (*It == TargetState)
        {
            delete *It;
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

FAnimStateTransition* UAnimationStateMachine::AddTransition
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
        UE_LOG("[UAnimationStateMachine::AddTransition] Warning : The target state you are trying to connect with this transition does not exist in the state machine.");
        return nullptr;
    }

    // 이미 같은 노드에 대한 연결이 있으면 거부
    for (const FAnimStateTransition* Transition : Transitions)
    {
        if (Transition->SourceState == SourceState &&
            Transition->TargetState == TargetState)
        {
            UE_LOG("[UAnimationStateMachine::AddTransition] Warning : A transition to the same target state already exists in the state machine.");
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

FAnimStateTransition* UAnimationStateMachine::AddTransition
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
        UE_LOG("[UAnimationStateMachine::AddTransition] Warning : TransitionRule is nullptr.");
        return Transition;
    }

    // Rule 연결 및 Delegate 바인딩
    Transition->AssociatedRule = TransitionRule;
    Transition->DelegateHandle = TransitionRule->GetTransitionDelegate().AddDynamic(
        Transition, &FAnimStateTransition::TriggerTransition);

    return Transition;
}

void UAnimationStateMachine::DeleteTransition(const FName& SourceName, const FName& TargetName)
{
    // 입력된 이름과 일치하는 State 찾기
    FAnimState* SourceState = FindStateByName(SourceName);
    FAnimState* TargetState = FindStateByName(TargetName);

    if (!SourceState || !TargetState)
    {
        UE_LOG("[UAnimationStateMachine::DeleteTransition] Warning : The transition you are trying to delete does not exist in the state machine.");
        return;
    }

    for (auto It = Transitions.begin(); It != Transitions.end(); It++)
    {
        if ((*It)->SourceState == SourceState &&
            (*It)->TargetState == TargetState)
        {
            (*It)->CleanupDelegate();
            delete *It;
            Transitions.erase(It);
            return;
        }
    }
    UE_LOG("[UAnimationStateMachine::DeleteTransition] Warning : The transition you are trying to delete does not exist in the state machine.");
}
