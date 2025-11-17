#pragma once
#include "AnimationSequence.h"
#include "Delegates.h"
#include "AnimNodeTransitionRule.h"

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

struct FAnimNode_TwoWayBlend : FAnimNode_Base
{
    FAnimNode_Base* From = nullptr;
    FAnimNode_Base* To = nullptr;

    float Alpha = 0.f;        // 0 → A, 1 → B
    float BlendTime = 0.2f;
    float BlendTimeElapsed = 0.f;
    bool bIsBlending = false;

    virtual void Update(const FAnimationUpdateContext& Context) override
    {
        if (From) From->Update(Context);
        if (To) To->Update(Context);

        if (bIsBlending)
        {
            BlendTimeElapsed += Context.DeltaTime;
            Alpha = FMath::Clamp(BlendTimeElapsed / BlendTime, 0.f, 1.f);

            if (BlendTimeElapsed >= BlendTime)
                bIsBlending = false;
        }
    }

    virtual void Evaluate(FPoseContext& Output) override
    {
        if (!From || !To)
            return;

        FPoseContext PoseFrom(Output.Skeleton);
        FPoseContext PoseTo(Output.Skeleton);

        From->Evaluate(PoseFrom);
        To->Evaluate(PoseTo);

        Blend(PoseFrom, PoseTo, Alpha, Output);
    }

    void Blend(const FPoseContext& Start, const FPoseContext& End, float Alpha, FPoseContext& Out)
    {
        const int32 BoneCnt = std::min(Start.EvaluatedPoses.Num(), End.EvaluatedPoses.Num());
        Out.EvaluatedPoses.SetNum(BoneCnt);

        const float ClampedWeight = std::clamp(Alpha, 0.0f, 1.0f);

        for (int i = 0; i < BoneCnt; i++)
        {
            Out.EvaluatedPoses[i] = FTransform::Lerp(Start.EvaluatedPoses[i], End.EvaluatedPoses[i], Alpha);
        }
    }
};
struct FAnimState
{
    FName Name{};
    uint32 Index{};   // Animation State Machine에서의 Index
    FAnimNode_Base* EntryNode = nullptr;
    TArray<FAnimNode_Base*> Nodes;

    void SetEntryNode(FAnimNode_Base* InNode)
    {
        EntryNode = InNode;
    }

    template<typename TNode>
    TNode* AddNode(TNode* InNode)
    {
        Nodes.Add(InNode);
        return InNode;
    }
};

struct FAnimStateTransition
{
    FAnimState* SourceState = nullptr;
    FAnimState* TargetState = nullptr;

    uint32 Index{}; // Animation State Machine에서의 Index

    bool CanEnterTransition = false; // true가 되면 Transition 발동

    // Delegate 관리
    UAnimNodeTransitionRule* AssociatedRule = nullptr;
    FDelegateHandle DelegateHandle;

    float BlendTime = 0.2f;       // ActiveState Pose -> TargetState Pose로 자연스럽게 Blend

    FAnimStateTransition() = default;
    FAnimStateTransition(const FAnimStateTransition& Other);
    FAnimStateTransition& operator=(const FAnimStateTransition& Other);
    ~FAnimStateTransition();

    void CleanupDelegate();

    void TriggerTransition() { CanEnterTransition = true; } // 특정 조건을 충족하면 외부의 delegate에서 호출

    void SetBlendTime(float InBlendTime) { BlendTime = InBlendTime; }
};

struct FAnimNode_StateMachine : FAnimNode_Base 
{
    TArray<FAnimState*> States;
    TArray<FAnimStateTransition*> Transitions;

    FAnimState* CurrentState = nullptr;

    // Transition 중인지 여부
    bool bIsInTransition = false;
    FAnimStateTransition* CurrentTransition = nullptr;

    float TransitionElapsed = 0.f;
    float TransitionDuration = 0.f;
    FAnimNode_TwoWayBlend TransitionBlendNode;

    virtual ~FAnimNode_StateMachine();

    virtual void Update(const FAnimationUpdateContext& Context) override;
    virtual void Evaluate(FPoseContext& Output) override;

    // --------- 상태 API ----------
    /**
     * @brief 새로운 State 추가
     * @param StateName 추가할 State의 이름
     * @return 추가된 State의 포인터
     */
    FAnimState* AddState(const FName& StateName);
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