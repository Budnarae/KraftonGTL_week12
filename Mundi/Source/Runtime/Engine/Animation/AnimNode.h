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

    float GetLength() const { return Sequence ? Sequence->GetPlayLength() : 0.f; }
    float GetCurrentTime() const { return CurrentTime; }

    float GetNormalizedTime() const
    {
        float Len = GetLength();
        return (Len > 0.f) ? (CurrentTime / Len) : 0.f;
    }
    void SetNormalizedTime(float Normalized)
    {
        float Len = GetLength();
        if (Len > 0.f)
            CurrentTime = FMath::Clamp(Normalized, 0.f, 1.f) * Len;
        else
            CurrentTime = 0.f;
    }

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

struct FBlendSample1D
{
    float Position = 0.0f; // 이 샘플이 놓인 위치 (ex: 속도 0, 200, 500)
    FAnimNode_Sequence* SequenceNode = nullptr; // 재생할 노드
    float Weight = 0.0f;
};

struct FAnimNode_BlendSpace1D : public FAnimNode_Base
{
    float BlendInput = 0.0f; // 외부 세팅 값 (ex: 이동 속도)
    float MinimumPosition = 0.0f; // BlendSpace 시작값
    float MaximumPosition = 1.0f; // BlendSpace 끝값
    bool IsTimeSynchronized = true; // 샘플 시퀀스들끼리 시간 동기화 여부

    TArray <FBlendSample1D> Samples;

    void AddSample(FAnimNode_Sequence* SequenceNode, float Position)
    {
        FBlendSample1D Sample;
        Sample.Position = Position;
        Sample.SequenceNode = SequenceNode;
        Sample.Weight = 0.0f;
        Samples.Add(Sample);

        Samples.Sort([](const FBlendSample1D& A, const FBlendSample1D& B)
        {
            return A.Position < B.Position;
        });
    }

    void SetBlendInput(float InValue) { BlendInput = InValue; }

    virtual void Update(const FAnimationUpdateContext& Context) override;
    virtual void Evaluate(FPoseContext& Output) override;

private:
    void CalculateSampleWeights();
    void SynchronizeSampleTimes();
};

struct FAnimState
{
    FName Name{};
    uint32 Index{};
    FAnimNode_Base* EntryNode = nullptr;
    TArray<FAnimNode_Base*> OwnedNodes;

    template<typename TNode, typename... Args>
    TNode* CreateNode(Args&&... args)
    {
        TNode* Node = new TNode(std::forward<Args>(args)...);
        OwnedNodes.Add(Node);
        return Node;
    }

    FAnimNode_Base* GetEntryNode() const { return EntryNode; }

    void SetEntryNode(FAnimNode_Base* Node)
    {
        EntryNode = Node;
    }

    void ResetNodes()
    {
        for (FAnimNode_Base* Node : OwnedNodes)
        {
            delete Node;
        }
        OwnedNodes.Empty();
        EntryNode = nullptr;
    }

    FAnimNode_Sequence* CreateSequenceNode(UAnimationSequence* Sequence, bool bLoop = true)
    {
        FAnimNode_Sequence* Node = CreateNode<FAnimNode_Sequence>();
        if (Node)
        {
            Node->SetSequence(Sequence, bLoop);
            Node->SetLooping(bLoop);
        }
        return Node;
    }

    FAnimNode_BlendSpace1D* CreateBlendSpace1DNode()
    {
        return CreateNode<FAnimNode_BlendSpace1D>();
    }
};

struct FAnimStateTransition
{
    FAnimState* SourceState = nullptr;
    FAnimState* TargetState = nullptr;

    uint32 Index{}; // Animation State Machine에서의 Index

    bool CanEnterTransition = false; // true가 되면 Transition 발동

    // Lua 함수 기반 Transition 조건
    sol::function TransitionConditionFunc;

    float BlendTime = 0.2f;       // ActiveState Pose -> TargetState Pose로 자연스럽게 Blend

    FAnimStateTransition() = default;
    FAnimStateTransition(const FAnimStateTransition& Other);
    FAnimStateTransition& operator=(const FAnimStateTransition& Other);

    void TriggerTransition() { CanEnterTransition = true; }

    void SetBlendTime(float InBlendTime) { BlendTime = InBlendTime; }

    /**
     * @brief Transition 조건 함수 설정 (Lua에서 호출)
     * @param InFunc Lua 함수 (bool 반환)
     */
    void SetTransitionCondition(sol::function InFunc)
    {
        TransitionConditionFunc = InFunc;
    }

    /**
     * @brief Transition 조건 평가 (Lua 함수 호출)
     * @return 조건 함수의 반환값 (bool)
     */
    bool EvaluateCondition()
    {
        if (TransitionConditionFunc.valid())
        {
            sol::protected_function_result result = TransitionConditionFunc();
            if (result.valid())
            {
                sol::optional<bool> value = result;
                if (value)
                {
                    return value.value();
                }
            }
        }
        return false;
    }

    /**
     * @brief Update: Transition 조건 평가
     */
    void Update(const FAnimationUpdateContext& Context)
    {
        // Lua 함수 기반 조건 평가
        if (TransitionConditionFunc.valid())
        {
            CanEnterTransition = EvaluateCondition();
        }
    }
};

struct FAnimNode_StateMachine : FAnimNode_Base
{
    TArray<FAnimState*> States;
    TArray<FAnimStateTransition*> Transitions;

    FAnimState* CurrentState = nullptr;

    bool bUseTransitionBlend = true;  // Transition 중 Blend 할지
    bool bIsInTransition = false; // Transition 중인지 여부
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
     * @brief Source와 Target의 이름으로 새로운 Transition 추가
     * @return 추가된 Transition의 포인터
     */
    FAnimStateTransition* AddTransition
    (
        const FName& SourceName,
        const FName& TargetName
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

    /**
     * @brief Reset CanEnterTransition for all transitions
     */
    void ResetTransitionFlags();

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



