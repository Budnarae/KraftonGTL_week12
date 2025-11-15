#pragma once

class UAnimationStateMachine : public UObject
{
    DECLARE_CLASS(UAnimationStateMachine, UObject)
public:
    /**
     * @brief 내부 상태 업데이트 (StateMachine 전이, 시퀀스 재생 시간 증가 등)
     * @return 없음
     */
    void Update(const FAnimationUpdateContext& Context);

    /**
     * @brief 이 노드가 생성하는 최종 Pose 계산(모든 애니메이션 노드는 반드시 이 함수를 통해 Pose를 출력)
     * @return 인자로 받는 Output을 갱신
     */
    void Evaluate(FPoseContext& Output);

    /**
     * @brief 새로운 State 추가
     * @return 없음
     */
    void AddState(const FAnimState& NewState);

    /**
     * @brief 이름으로 State 제거
     * @return 없음
     */
    void DeleteState(const FName& TargetName);

    /**
     * @brief Source와 Target의 이름으로 새로운 Transition 추가
     * @return 없음
     */
    void AddTransition
    (
        const FName& SourceName,
        const FName& TargetName,
        const FAnimStateTransition& NewTransition
    );

    /**
     * @brief Source와 Target의 이름으로 Transition 제거
     * @return 없음
     */
    void DeleteTransition(const FName& SourceName, const FName& TargetName);

private:
    inline const static int32 INVALID = -1;
    
    // 현재 활성화된 State Index
    int32 CurrentStateIndex = INVALID;
    int32 CurrentTransitionIndex = INVALID;

    // Transition 중인지 여부
    bool bIsInTransition = false;
    
    // State 리스트 (각 State는 내부에 AnimNode 포함)
    TArray<FAnimState> States;

    // Transition 리스트
    TArray<FAnimStateTransition> Transitions;

    /* 아래 요소가 필요하다면 주석을 해제하세요 */
    
    // // 이전 State index (Transition 중 사용할 수 있음)
    // int32 PreviousStateIndex{};
    //
    // // State 체류 시간
    // float StateElapsedTime;
};