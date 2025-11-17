#pragma once

class UAnimationStateMachine : public UObject
{
    DECLARE_CLASS(UAnimationStateMachine, UObject)

public:
    UAnimationStateMachine();
    ~UAnimationStateMachine();
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
     * @param StateName 추가할 State의 이름
     * @return 추가된 State의 포인터
     */
    FAnimState* AddState(const FName& StateName);

    /**
     * @brief 이름으로 State 제거
     * @return 없음
     */
    void DeleteState(const FName& TargetName);

    /**
     * @brief Source와 Target의 이름으로 새로운 Transition 추가 (기본 버전)
     * @return 추가된 Transition의 포인터
     */
    FAnimStateTransition* AddTransition
    (
        const FName& SourceName,
        const FName& TargetName
    );

    /**
     * @brief Source와 Target의 이름으로 새로운 Transition 추가 및 Rule 연결
     * @param SourceName 출발 State 이름
     * @param TargetName 도착 State 이름
     * @param TransitionRule Transition 조건을 판단할 Rule
     * @return 추가된 Transition의 포인터
     */
    FAnimStateTransition* AddTransition
    (
        const FName& SourceName,
        const FName& TargetName,
        UAnimNodeTransitionRule* TransitionRule
    );

    /**
     * @brief Source와 Target의 이름으로 Transition 제거
     * @return 없음
     */
    void DeleteTransition(const FName& SourceName, const FName& TargetName);

    /**
     * @brief 현재 State 반환
     * @return 현재 State 포인터
     */
    FAnimState* GetCurrentState() const { return CurrentState; }

    /**
     * @brief Transitions 배열 접근
     * @return Transitions 참조
     */
    TArray<FAnimStateTransition*>& GetTransitions() { return Transitions; }

private:
    /**
     * @brief 이름으로 State를 찾습니다
     * @return State 포인터, 찾지 못하면 nullptr
     */
    FAnimState* FindStateByName(const FName& StateName) const;

private:
    // 현재 활성화된 State (포인터)
    FAnimState* CurrentState = nullptr;
    FAnimStateTransition* CurrentTransition = nullptr;

    // Transition 중인지 여부
    bool bIsInTransition = false;

    // State 리스트 (동적 할당)
    TArray<FAnimState*> States;

    // Transition 리스트 (동적 할당)
    TArray<FAnimStateTransition*> Transitions;

    /* 아래 요소가 필요하다면 주석을 해제하세요 */
    
    // // 이전 State index (Transition 중 사용할 수 있음)
    // int32 PreviousStateIndex{};
    //
    // // State 체류 시간
    // float StateElapsedTime;
};