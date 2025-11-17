#pragma once
#include "AnimationType.h"
#include "AnimationSequence.h"

struct FAnimNode_Base {
	virtual void Update(const FAnimationUpdateContext& Context) = 0;
	virtual void Evaluate(FPoseContext& Output) = 0;
	virtual ~FAnimNode_Base() = default;
};

struct FAnimNode_Sequence : FAnimNode_Base
{
    UAnimationSequence* Sequence = nullptr;
    float CurrentTime = 0.0f;
    float PlayRate = 1.0f;
    bool bLooping = true;

    void SetSequence(UAnimationSequence* InSeq, bool bInLoop = true)
    {
        Sequence = InSeq;
        bLooping = bInLoop;
        CurrentTime = 0.0f;
    }

    void SetLooping(bool bInLooping) { bLooping = bInLooping; }

    virtual void Update(const FAnimationUpdateContext& Context) override
    {
        if (!Sequence)
            return;

        const float Length = Sequence->GetPlayLength();
        if (Length <= 0.0f)
            return;

        CurrentTime += Context.DeltaTime * PlayRate;

        if (bLooping)
        {
            CurrentTime = std::fmod(CurrentTime, Length);
            if (CurrentTime < 0.0f)
            {
                CurrentTime += Length;
            }
        }
        else
        {
            CurrentTime = FMath::Clamp(CurrentTime, 0.0f, Length);
        }
    }

    virtual void Evaluate(FPoseContext& Output) override
    {
        if (!Sequence)
            return;

        Sequence->EvaluatePose(CurrentTime, Output);
    }
};

class UAnimationSequence;

struct FAnimState
{
    FName Name{};
    uint32 Index{};   // Animation State Machine에서의 Index
    TArray<FAnimNode_Sequence> AnimSequenceNodes;

    /**
     * @brief AnimSequence를 이 State에 추가
     */
    FAnimNode_Sequence* AddAnimSequence(UAnimationSequence* AnimSequence, bool bLoop = true)
    {
        if (!AnimSequence)
        {
            return nullptr;
        }

        AnimSequenceNodes.Emplace();
        FAnimNode_Sequence& Node = AnimSequenceNodes.Last();
        Node.SetSequence(AnimSequence, bLoop);
        return &Node;
    }
};

class UAnimNodeTransitionRule;
struct FAnimNode_StateMachine : FAnimNode_Base 
{
    // 현재 활성화된 State (포인터)
    FAnimState* CurrentState = nullptr;
    FAnimStateTransition* CurrentTransition = nullptr;

    // Transition 중인지 여부
    bool bIsInTransition = false;

    // State 리스트 (동적 할당)
    TArray<FAnimState*> States;

    // Transition 리스트 (동적 할당)
    TArray<FAnimStateTransition*> Transitions;

    /**
    * @brief 내부 상태 업데이트 (StateMachine 전이, 시퀀스 재생 시간 증가 등)
    * @return 없음
    */
    virtual void Update(const FAnimationUpdateContext& Context) override;

    /**
     * @brief 이 노드가 생성하는 최종 Pose 계산(모든 애니메이션 노드는 반드시 이 함수를 통해 Pose를 출력)
     * @return 인자로 받는 Output을 갱신
     */
    virtual void Evaluate(FPoseContext& Output) override;


    // --------- 상태 API ----------
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

    // --------- 트랜지션 API ----------
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

    // ----- 조회 -----
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

    /* 아래 요소가 필요하다면 주석을 해제하세요 */

    // // 이전 State index (Transition 중 사용할 수 있음)
    // int32 PreviousStateIndex{};
    //
    // // State 체류 시간
    // float StateElapsedTime;
};
