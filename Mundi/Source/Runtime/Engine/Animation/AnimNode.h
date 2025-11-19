#pragma once
#include "AnimationSequence.h"
#include "AnimNotify/AnimNotifyState.h"

enum class EAnimBlendEaseType : uint8
{
    Linear = 0,
    EaseIn,
    EaseOut,
    EaseInOut
};

float ApplyAnimBlendEase(float Alpha, EAnimBlendEaseType EaseType);

struct FAnimNode_Base {
	virtual void Update(const FAnimationUpdateContext& Context) = 0;
	virtual void Evaluate(FPoseContext& Output) = 0;
	virtual ~FAnimNode_Base() = default;
};

struct FAnimNode_Sequence : FAnimNode_Base
{
    UAnimationSequence* Sequence = nullptr;
    float CurrentTime = 0.0f;
    float LastTime = 0.0f;
    float PlayRate = 1.0f;
    bool bLooping = true;
    bool IsUsingSoundNotify = true;

    void EndPlay()
    {
        CurrentTime = 0.f;
        LastTime = 0.f;

        for (UAnimNotifyState* AnimNotifyState : Sequence->GetAnimNotifyStates())
        {
            if (!AnimNotifyState->GetEndAlreadyCalled())
                AnimNotifyState->NotifyEnd();
        }
    }

    void SetSequence(UAnimationSequence* InSeq, bool bInLoop = true)
    {
        Sequence = InSeq;
        bLooping = bInLoop;
        CurrentTime = 0.0f;
        LastTime = 0.0f;
    }

    void SetLooping(bool bInLooping) { bLooping = bInLooping; }
    void SetPlayRate(float TargetPlayTime)
    {
        if (TargetPlayTime == 0.0) return;

        PlayRate = GetLength() / TargetPlayTime;
    }
    void SetReversePlay()
    {
        PlayRate = -1.f;
    }

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

        LastTime = CurrentTime;

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

    EAnimBlendEaseType EaseFunction = EAnimBlendEaseType::Linear;

    float Alpha = 0.f;        // 0 → A, 1 → B
    float BlendTime = 0.2f;
    float BlendTimeElapsed = 0.f;
    bool bIsBlending = false;
    bool bHasCachedBlendEndpoints = false;

    FPoseContext CachedFromPose;
    FPoseContext CachedToPose;

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
        else
        {
            bHasCachedBlendEndpoints = false;
            CachedFromPose.EvaluatedPoses.SetNum(0);
            CachedToPose.EvaluatedPoses.SetNum(0);
        }
    }

    virtual void Evaluate(FPoseContext& Output) override
    {
        if (!From || !To)
            return;

         // 기존 방식: 매 프레임에서 From/To를 평가하고 섞는다.
         FPoseContext PoseFrom(Output.Skeleton);
         FPoseContext PoseTo(Output.Skeleton);
         From->Evaluate(PoseFrom);
         To->Evaluate(PoseTo);
         Blend(PoseFrom, PoseTo, Alpha, Output);

        //if (bIsBlending && !bHasCachedBlendEndpoints)
        //{
        //    CacheBlendEndpoints(Output);
        //}

        //if (!bHasCachedBlendEndpoints)
        //{
        //    return;
        //}

        //const int32 BoneCnt = std::min(CachedFromPose.EvaluatedPoses.Num(), CachedToPose.EvaluatedPoses.Num());
        //Output.EvaluatedPoses.SetNum(BoneCnt);

        //const float ClampedWeight = std::clamp(Alpha, 0.0f, 1.0f);
        //for (int i = 0; i < BoneCnt; i++)
        //{
        //    Output.EvaluatedPoses[i] = FTransform::Lerp(CachedFromPose.EvaluatedPoses[i], CachedToPose.EvaluatedPoses[i], ClampedWeight);
        //}
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

    void CacheBlendEndpoints(const FPoseContext& Output)
    {
        if (!Output.Skeleton)
        {
            bHasCachedBlendEndpoints = false;
            return;
        }

        CachedFromPose = FPoseContext(Output.Skeleton);
        CachedToPose = FPoseContext(Output.Skeleton);

        From->Evaluate(CachedFromPose);
        To->Evaluate(CachedToPose);

        bHasCachedBlendEndpoints = true;
    }

    void ResetCachedBlend()
    {
        bHasCachedBlendEndpoints = false;
        CachedFromPose.EvaluatedPoses.SetNum(0);
        CachedToPose.EvaluatedPoses.SetNum(0);
    }
};
struct FBlendSample1D
{
    float Position = 0.0f; // 이 샘플이 놓인 위치 (ex: 속도 0, 200, 500)
    FAnimNode_Sequence* SequenceNode = nullptr; // 재생할 노드
    float Weight = 0.0f;
};

struct FBlendSample2D
{
    int32 GridXIndex = 0;
    int32 GridYIndex = 0;
    FAnimNode_Sequence* SequenceNode = nullptr;
    float Weight = 0.0f;
};

struct FAnimNode_BlendSpace1D : public FAnimNode_Base
{
    EAnimBlendEaseType EaseFunction = EAnimBlendEaseType::Linear;

    float BlendInput = 0.0f; // 외부 세팅 값 (ex: 이동 속도)
    float MinimumPosition = 0.0f; // BlendSpace 시작값
    float MaximumPosition = 1.0f; // BlendSpace 끝값
    bool bIsTimeSynchronized = false; // 샘플 시퀀스들끼리 시간 동기화 여부

    bool bHasManualMin = false;
    bool bHasManualMax = false;

    TArray <FBlendSample1D> Samples;

    void SetBlendInput(float InValue) { BlendInput = InValue; }
    void SetMinimumPosition(float InValue) { MinimumPosition = InValue; bHasManualMin = true; }
    void SetMaximumPosition(float InValue) { MaximumPosition = InValue; bHasManualMax = true; }

    virtual void Update(const FAnimationUpdateContext& Context) override;
    virtual void Evaluate(FPoseContext& Output) override;

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

        UpdateRangeFromSamples();
    }

private:
    void CalculateSampleWeights();
    void SimpleSynchronizeSampleTimes();
    void SynchronizeSampleTimes();
    void UpdateRangeFromSamples();
};


struct FAnimNode_BlendSpace2D : public FAnimNode_Base
{
    EAnimBlendEaseType EaseFunction = EAnimBlendEaseType::Linear;

    float BlendInputX = 0.0f;
    float BlendInputY = 0.0f;

    // 축 정의 : 1D처럼 단순히 Min, Max만 설정하는 게 아니라, 구간을 설정해야하기 때문
    // => 오름차순 정렬
    TArray<float> GridXValues; 
    TArray<float> GridYValues;

    TArray <FBlendSample2D> Samples;

    bool bIsTimeSynchronized = false;

    void SetBlendInput(float InX, float InY)
    {
        BlendInputX = InX;
        BlendInputY = InY;
    }
    
    void SetGridAxes(const TArray<float>& InXValues, const TArray<float>& InYValues)
    {
        GridXValues = InXValues;
        GridYValues = InYValues;

        const int32 NumX = GridXValues.Num();
        const int32 NumY = GridYValues.Num();

        Samples.SetNum(NumX * NumY);
        for (int32 YIndex = 0; YIndex < NumY; ++YIndex)
        {
            for (int32 XIndex = 0; XIndex < NumX; ++XIndex)
            {
                const int32 SampleIndex = XIndex + YIndex * NumX;
                Samples[SampleIndex].GridXIndex = XIndex;
                Samples[SampleIndex].GridYIndex = YIndex;
            }
        }
    }

    void AddSample(FAnimNode_Sequence* SequenceNode, int32 XIndex, int32 YIndex)
    {
        const int32 NumX = GridXValues.Num();
        const int32 NumY = GridYValues.Num();

        if (XIndex < 0 || XIndex >= NumX || YIndex < 0 || YIndex >= NumY) { return; }

        const int32 SampleIndex = XIndex + NumX * YIndex;
        Samples[SampleIndex].SequenceNode = SequenceNode;
    }

    virtual void Update(const FAnimationUpdateContext& Context) override;
    virtual void Evaluate(FPoseContext& Output) override;

private:
    void CalculateSampleWeights();
    void AdvancedCalculateSampleWeights(); // 사실 상 안 쓰임! 쓸 사람 고쳐서 쓰셈.
    void SynchronizeSampleTimes();
    void SimpleSynchronizeSampleTimes();
};

struct FAnimNode_AdditiveBlend : public FAnimNode_Base
{
    FAnimNode_Base* BasePose = nullptr;
    FAnimNode_Base* AdditivePose = nullptr;

    float Alpha = 1.0f;

    virtual void Update(const FAnimationUpdateContext& Context) override
    {
        if (BasePose) BasePose->Update(Context);
        if (AdditivePose) AdditivePose->Update(Context);
    }

    virtual void Evaluate(FPoseContext& Output) override
    {
        FPoseContext Base(Output);
        FPoseContext Add(Output);

        BasePose->Evaluate(Base);
        AdditivePose->Evaluate(Add);

        ApplyAdditive(Base, Add, Alpha, Output);
    }

    void ApplyAdditive(const FPoseContext& Base, const FPoseContext& Add, float Alpha, FPoseContext& Out)
    {
        const int32 BoneCount = Base.EvaluatedPoses.Num();
        Out.EvaluatedPoses.SetNum(BoneCount);

        // 기존 구현 (Lerp 방식 - 런타임에 차이 계산)
        // Base를 Reference Pose로 간주하고 차이를 계산 후 적용
        // Result = Base + ((Additive - Base) * Alpha) = Lerp(Base, Additive, Alpha)
        // for (int i = 0; i < BoneCount;i++)
        // {
        //     const FTransform& BTrans = Base.EvaluatedPoses[i];
        //     const FTransform& ATrans = Add.EvaluatedPoses[i];
        //
        //     const FQuat DeltaRot = ATrans.Rotation * BTrans.Rotation.Inverse();
        //     const FVector DeltaPos = (ATrans.Translation - BTrans.Translation);
        //     const FVector DeltaScale = (ATrans.Scale3D / BTrans.Scale3D);
        //
        //     FQuat WeightedDelta = FQuat::Slerp(FQuat::Identity(), DeltaRot, Alpha);
        //     Out.EvaluatedPoses[i].Rotation = WeightedDelta * BTrans.Rotation;
        //
        //     Out.EvaluatedPoses[i].Translation = BTrans.Translation + (DeltaPos * Alpha);
        //
        //     Out.EvaluatedPoses[i].Scale3D = BTrans.Scale3D * FVector::Lerp(FVector(1, 1, 1), DeltaScale, Alpha);
        // }

        // // 새로운 구현 (진짜 Additive - Add가 이미 계산된 차이 포즈라고 가정)
        // // Add = SourceAnimation - ReferencePose (오프라인에서 계산됨)
        // // Result = (1 - Alpha) * Target + Alpha * (Target + Additive)
        // //        = Target + Alpha * Additive
        for (int i = 0; i < BoneCount; i++)
        {
            const FTransform& TargetTrans = Base.EvaluatedPoses[i];
            const FTransform& AdditiveTrans = Add.EvaluatedPoses[i];
        
            // Rotation: (1 - Alpha) * Target + Alpha * (Target * Additive)
            // = Target * Slerp(Identity, Additive, Alpha)
            FQuat AdditiveRot = FQuat::Slerp(FQuat::Identity(), AdditiveTrans.Rotation, Alpha);
            Out.EvaluatedPoses[i].Rotation = TargetTrans.Rotation * AdditiveRot;
        
            // Translation: (1 - Alpha) * Target + Alpha * (Target + Additive)
            // = Target + Alpha * Additive
            Out.EvaluatedPoses[i].Translation = TargetTrans.Translation + (AdditiveTrans.Translation * Alpha);
        
            // Scale: (1 - Alpha) * Target + Alpha * (Target * Additive)
            // = Target * Lerp(1, Additive, Alpha)
            FVector AdditiveScale = FVector::Lerp(FVector(1, 1, 1), AdditiveTrans.Scale3D, Alpha);
            Out.EvaluatedPoses[i].Scale3D = TargetTrans.Scale3D * AdditiveScale;
        }
    }
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

    FAnimNode_BlendSpace2D* CreateBlendSpace2DNode()
    {
        return CreateNode<FAnimNode_BlendSpace2D>();
    }

    FAnimNode_AdditiveBlend* CreateAdditiveBlendNode()
    {
        return CreateNode<FAnimNode_AdditiveBlend>();
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
};
