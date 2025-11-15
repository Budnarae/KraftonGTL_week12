#include "pch.h"
#include "AnimationStateMachine.h"
#include "AnimationSequence.h"

#include "Keyboard.h"

IMPLEMENT_CLASS(UAnimationStateMachine)

int32 UAnimationStateMachine::FindStateIndexByName(const FName& StateName) const
{
    for (int32 i = 0; i < States.Num(); i++)
    {
        if (States[i].Name == StateName)
            return i;
    }
    return -1;
}
/**
 * @brief 내부 상태 업데이트 (StateMachine 전이, 시퀀스 재생 시간 증가 등)
 * @return 없음
 */
void UAnimationStateMachine::Update(const FAnimationUpdateContext& Context)
{
    if (bIsInTransition)
    {
        Transitions[CurrentTransitionIndex].Update(Context);

        // Transition 완료 처리
        if (!Transitions[CurrentTransitionIndex].bIsBlending)
        {
            CurrentStateIndex = Transitions[CurrentTransitionIndex].TargetStateIndex;
            bIsInTransition = false;
            Transitions[CurrentTransitionIndex].CanEnterTransition = false;
        }
    }
    else
    {
        if (CurrentStateIndex == INVALID ||
            CurrentStateIndex >= States.Num() ||
            States[CurrentStateIndex].AnimSequences.empty())
            return;

        // 조건을 충족한 Transition이 있으면 State를 변환한다.
        for (int32 i = 0; i < Transitions.Num(); i++)
        {
            if (Transitions[i].CanEnterTransition &&
                Transitions[i].SourceStateIndex == CurrentStateIndex)
            {
                CurrentTransitionIndex = i;
                bIsInTransition = true;
                return;
            }
        }

        // 블렌드 구현 이전까지 State의 첫번째 Sequence를 우선하여 적용합니다.
        UAnimationSequence* SequenceToUpdate = States[CurrentStateIndex].AnimSequences.front();
        SequenceToUpdate->Update(Context);
    }
}

/**
 * @brief 이 노드가 생성하는 최종 Pose 계산(모든 애니메이션 노드는 반드시 이 함수를 통해 Pose를 출력)
 * @return 인자로 받는 Output을 갱신
 */
void UAnimationStateMachine::Evaluate(FPoseContext& Output)
{
    // 추후 구현
    if (bIsInTransition)
    {
        Transitions[CurrentTransitionIndex].Evaluate(Output);
    }
    else
    {
        if (CurrentStateIndex == INVALID ||
            CurrentStateIndex >= States.Num() ||
            States[CurrentStateIndex].AnimSequences.empty())
            return;
        // 블렌드 구현 이전까지 State의 첫번째 Sequence를 우선하여 적용합니다.
        UAnimationSequence* SequenceToEvaluate = States[CurrentStateIndex].AnimSequences.front();
        SequenceToEvaluate->Evaluate(Output);
    }
}

/**
 * @brief 새로운 State 입력
 * @return 없음
 */
void UAnimationStateMachine::AddState(const FAnimState& NewState)
{
    // 비어있는 그래프였다면 유효한 그래프로 전환
    if (CurrentStateIndex == INVALID) CurrentStateIndex = 0;

    // 같은 이름을 가진 State가 있으면 거부
    for (const FAnimState& State : States)
    {
        if (State.Name == NewState.Name)
        {
            UE_LOG("[UAnimationStateMachine::AddState] Warning : A state with the same name already exists in the Animation State Machine.");
            return;
        }
    }

    States.push_back(NewState);
    States.back().Index = States.Num() - 1;
}

/**
 * @brief 이름으로 State 제거
 * @return 없음
 */
void UAnimationStateMachine::DeleteState(const FName& TargetName)
{
    // 삭제할 State의 인덱스 찾기
    int32 TargetStateIndex = FindStateIndexByName(TargetName);

    if (TargetStateIndex == -1)
    {
        UE_LOG("[UAnimationStateMachine::DeleteState] Warning : The state you are trying to delete does not exist in the state machine.");
        return;
    }

    // 해당 State를 참조하는 Transition들을 먼저 제거
    for (int32 i = Transitions.Num() - 1; i >= 0; i--)
    {
        if (Transitions[i].SourceStateIndex == TargetStateIndex ||
            Transitions[i].TargetStateIndex == TargetStateIndex)
        {
            Transitions.erase(Transitions.begin() + i);
        }
    }

    // State 제거
    States.erase(States.begin() + TargetStateIndex);
}

/**
 * @brief Source와 Target의 이름으로 새로운 Transition 추가
 * @return 없음
 */
void UAnimationStateMachine::AddTransition
(
    const FName& SourceName,
    const FName& TargetName,
    const FAnimStateTransition& NewTransition
)
{
    // 입력된 이름과 일치하는 State 인덱스 찾기
    int32 SourceStateIndex = FindStateIndexByName(SourceName);
    int32 TargetStateIndex = FindStateIndexByName(TargetName);

    if (SourceStateIndex == -1 || TargetStateIndex == -1)
    {
        UE_LOG("[UAnimationStateMachine::AddTransition] Warning : The target state you are trying to connect with this transition does not exist in the state machine.");
        return;
    }

    // 이미 같은 노드에 대한 연결이 있으면 거부
    for (const FAnimStateTransition& Transition : Transitions)
    {
        if (Transition.SourceStateIndex == SourceStateIndex &&
            Transition.TargetStateIndex == TargetStateIndex)
        {
            UE_LOG("[UAnimationStateMachine::AddTransition] Warning : A transition to the same target state already exists in the state machine.");
            return;
        }
    }

    Transitions.push_back(NewTransition);
    Transitions.back().SourceStateIndex = SourceStateIndex;
    Transitions.back().TargetStateIndex = TargetStateIndex;
}

/**
 * @brief Source와 Target의 이름으로 Transition 제거
 * @return 없음
 */
void UAnimationStateMachine::DeleteTransition(const FName& SourceName, const FName& TargetName)
{
    // 입력된 이름과 일치하는 State 인덱스 찾기
    int32 SourceStateIndex = FindStateIndexByName(SourceName);
    int32 TargetStateIndex = FindStateIndexByName(TargetName);

    if (SourceStateIndex == -1 || TargetStateIndex == -1)
    {
        UE_LOG("[UAnimationStateMachine::DeleteTransition] Warning : The transition you are trying to delete does not exist in the state machine.");
        return;
    }

    for (auto It = Transitions.begin(); It != Transitions.end(); It++)
    {
        if (It->SourceStateIndex == SourceStateIndex &&
            It->TargetStateIndex == TargetStateIndex)
        {
            Transitions.erase(It);
            return;
        }
    }
    UE_LOG("[UAnimationStateMachine::DeleteTransition] Warning : The transition you are trying to delete does not exist in the state machine.");
}