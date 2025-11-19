#include "pch.h"
#include "AnimNode.h"

float ApplyAnimBlendEase(float Alpha, EAnimBlendEaseType EaseType)
{
    Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
    switch (EaseType)
    {
        case EAnimBlendEaseType::EaseIn :
                return Alpha * Alpha;
        case EAnimBlendEaseType::EaseOut:
            {
                float Inv = 1.f - Alpha;
                return 1.f - Inv * Inv;
            }
        case EAnimBlendEaseType::EaseInOut:
            if (Alpha < 0.5f)
            {
                float Scaled = Alpha * 2.f;
                return 0.5f * Scaled * Scaled;
            }
            else
            {
                float Scaled = (1.f - Alpha) * 2.f;
                return 1.f - 0.5f * Scaled * Scaled;
            }
        default:
            return Alpha;
    }
}

// ====================================
// FAnimStateTransition 구현
// ====================================
// Note: Delegate 시스템 제거됨, Lua 함수 기반으로 단순화

FAnimStateTransition::FAnimStateTransition(const FAnimStateTransition& Other)
    : SourceState(Other.SourceState)
    , TargetState(Other.TargetState)
    , Index(Other.Index)
    , CanEnterTransition(Other.CanEnterTransition)
    , TransitionConditionFunc(Other.TransitionConditionFunc)  // Lua 함수 복사
    , BlendTime(Other.BlendTime)
{
}

FAnimStateTransition& FAnimStateTransition::operator=(const FAnimStateTransition& Other)
{
    if (this != &Other)
    {
        SourceState = Other.SourceState;
        TargetState = Other.TargetState;
        Index = Other.Index;
        CanEnterTransition = Other.CanEnterTransition;
        TransitionConditionFunc = Other.TransitionConditionFunc;  // Lua 함수 복사
        BlendTime = Other.BlendTime;
    }
    return *this;
}

// ====================================
// FAnimNode_StateMachine 구현
// ====================================

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
    for (FAnimStateTransition* Transition : Transitions)
    {
        if (Transition)
        {
            delete Transition;
        }
    }
    Transitions.Empty();

    for (FAnimState* State : States)
    {
        if (State)
        {
            State->ResetNodes();
            delete State;
        }
    }
    States.Empty();

    CurrentState = nullptr;
    CurrentTransition = nullptr;
}

void FAnimNode_StateMachine::Update(const FAnimationUpdateContext& Context)
{
    if (bIsInTransition)
    {
        if (!CurrentTransition || !TransitionBlendNode.From || !TransitionBlendNode.To)
        {
            bIsInTransition = false;
            CurrentTransition = nullptr;
            TransitionBlendNode.bIsBlending = false;
        }
        else
        {
            // 만약 Transition 중이라면 Cross Fade
            TransitionBlendNode.Update(Context);

            TransitionElapsed += Context.DeltaTime;
            float Alpha = (TransitionDuration > 0.f)
                ? FMath::Clamp(TransitionElapsed / TransitionDuration, 0.f, 1.f)
                : 1.f;

            Alpha = ApplyAnimBlendEase(Alpha, TransitionBlendNode.EaseFunction);
            TransitionBlendNode.Alpha = Alpha;

            if (Alpha >= 1.f)
            {
                if (CurrentTransition && CurrentTransition->TargetState)
                {
                    CurrentState = CurrentTransition->TargetState;
                }

                bIsInTransition = false;
                CurrentTransition = nullptr;
                TransitionElapsed = 0.f;
                TransitionDuration = 0.f;
            }

            return;
        }
    }

    if (!CurrentState || !CurrentState->EntryNode)
        return;

    for (FAnimStateTransition* Transition : Transitions)
    {
        if (!Transition)
            continue;

        // Transition Start!
        if (Transition->CanEnterTransition &&
            Transition->SourceState == CurrentState)
        {
            FAnimNode_Base* SourceEntry = CurrentState->EntryNode;
            FAnimNode_Base* TargetEntry = (Transition->TargetState) ? Transition->TargetState->EntryNode : nullptr;

            if (!SourceEntry || !TargetEntry)
            {
                continue;
            }

            CurrentTransition = Transition;
            bIsInTransition = true;
            TransitionElapsed = 0.f;
            TransitionDuration = Transition->BlendTime;

            if (auto* SourceSeq = dynamic_cast<FAnimNode_Sequence*>(SourceEntry))
            {
                if (auto* TargetSeq = dynamic_cast<FAnimNode_Sequence*>(TargetEntry))
                {
                    if (SourceSeq->Sequence && TargetSeq->Sequence)
                    {
                        const float SrcLen = SourceSeq->Sequence->GetPlayLength();
                        const float DstLen = TargetSeq->Sequence->GetPlayLength();
                        if (SrcLen > 0.f && DstLen > 0.f)
                        {
                            float Phase = SourceSeq->CurrentTime / SrcLen;
                            Phase = FMath::Clamp(Phase, 0.f, 1.f);
                            TargetSeq->CurrentTime = Phase * DstLen;
                        }
                    }
                }
            }

            TransitionBlendNode.From = SourceEntry;
            TransitionBlendNode.To = TargetEntry;
            TransitionBlendNode.Alpha = 0.f;
            TransitionBlendNode.BlendTime = Transition->BlendTime;
            TransitionBlendNode.BlendTimeElapsed = 0.0f;
            TransitionBlendNode.bIsBlending = true;
            bUseTransitionBlend = (SourceEntry && TargetEntry);

            return;
        }
    }

    CurrentState->EntryNode->Update(Context);
}
void FAnimNode_StateMachine::Evaluate(FPoseContext& Output)
{
    if (bIsInTransition)
    {
        if (TransitionBlendNode.From && TransitionBlendNode.To)
        {
            TransitionBlendNode.Evaluate(Output);
            return;
        }
    }

    if (!CurrentState || !CurrentState->EntryNode)
        return;

    CurrentState->EntryNode->Evaluate(Output);
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
            (*It)->ResetNodes();
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

void FAnimNode_StateMachine::ResetTransitionFlags()
{
    for (FAnimStateTransition* Transition : Transitions)
    {
        if (Transition)
        {
            Transition->CanEnterTransition = false;
        }
    }
}

void FAnimNode_BlendSpace1D::Update(const FAnimationUpdateContext& Context)
{
    if (Samples.Num() == 0) { return; }

    // 1) 가중치 계산
    CalculateSampleWeights();

    // 2) 시간 동기화
    // SynchronizeSampleTimes();
    SimpleSynchronizeSampleTimes();

    // 3) 가중치가 0이 아닌 시퀀스들만 Update
    for (FBlendSample1D& Sample : Samples)
    {
        if (Sample.Weight > KINDA_SMALL_NUMBER && Sample.SequenceNode)
        {
            Sample.SequenceNode->Update(Context);
        }
    }
}

void FAnimNode_BlendSpace1D::Evaluate(FPoseContext& Output)
{
    if (!Output.Skeleton)
    {
        // AnimInstance 쪽에서 미리 Skeleton을 세팅해줘야 한다.
        return;
    }

    if (Samples.Num() == 0) { Output.ResetToRefPose(); return; }

    FPoseContext SamplePose(Output); // 임시 포즈 버퍼 (Skeleton만 공유)

    const int32 NumBones = Output.EvaluatedPoses.Num();

    bool  bHasAnyPose = false;
    float TotalWeight = 0.0f;

    for (FBlendSample1D& Sample : Samples)
    {
        if (Sample.Weight <= KINDA_SMALL_NUMBER || Sample.SequenceNode == nullptr) { continue; }

        SamplePose.ResetToRefPose();
        Sample.SequenceNode->Evaluate(SamplePose);

        if (!bHasAnyPose)
        {
            // 첫 샘플이면 그대로 복사
            Output.EvaluatedPoses = SamplePose.EvaluatedPoses;
            TotalWeight = Sample.Weight;
            bHasAnyPose = true;
        }
        else
        {
            const float NewTotalWeight = TotalWeight + Sample.Weight;
            const float Alpha = Sample.Weight / NewTotalWeight;

            // 본별로 Lerp
            for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
            {
                Output.EvaluatedPoses[BoneIndex] =
                    FTransform::Lerp(
                        Output.EvaluatedPoses[BoneIndex],           // 지금까지 누적된 포즈
                        SamplePose.EvaluatedPoses[BoneIndex],       // 새 샘플 포즈
                        Alpha                                       // 새 샘플 비율
                    );
            }

            TotalWeight = NewTotalWeight;
        }
    }

    if (!bHasAnyPose)
    {
        // 모든 샘플 Weight가 0이면 기본 포즈
        Output.ResetToRefPose();
    }
}

void FAnimNode_BlendSpace1D::CalculateSampleWeights()
{
    if (Samples.Num() == 0) { return; }

    float ClampedInput = std::clamp(BlendInput, MinimumPosition, MaximumPosition);

    for (FBlendSample1D& Sample : Samples)
    {
        Sample.Weight = 0.0f;
    }

    if (Samples.Num() == 1) { Samples[0].Weight = 1.0f; return; }

    // 입력이 제일 왼쪽 이하일 때
    if (ClampedInput <= Samples[0].Position) { Samples[0].Weight = 1.0f; return; }

    // 입력이 제일 오른쪽 이상일 때
    if (ClampedInput >= Samples.Last().Position) { Samples.Last().Weight = 1.0f; return; }

    for (int32 Index = 0; Index < Samples.Num() - 1; ++Index)
    {
        const FBlendSample1D& LeftSample = Samples[Index];
        const FBlendSample1D& RightSample = Samples[Index + 1];

        // 두 시퀀스의 시간에 따른 비율로 Weight를 설정
        if (ClampedInput >= LeftSample.Position && ClampedInput <= RightSample.Position)
        {
            float Range = RightSample.Position - LeftSample.Position;
            float T = (Range > KINDA_SMALL_NUMBER)
                ? (ClampedInput - LeftSample.Position) / Range
                : 0.0f;

            Samples[Index].Weight = 1.0f - T;
            Samples[Index + 1].Weight = T;
            break;
        }
    }
}

void FAnimNode_BlendSpace1D::SimpleSynchronizeSampleTimes()
{
    if (bIsTimeSynchronized) { return; }
    bIsTimeSynchronized = true;

    float MaxPlayTime = 0;
    int32 MasterIndex = 0;
    for (int i = 0; i < Samples.Num(); i++)
    {
        float PlayTime = Samples[i].SequenceNode->GetLength();
        if (PlayTime > MaxPlayTime)
        {
            MaxPlayTime = PlayTime;
            MasterIndex = i;
        }   
    }
    
    for (int i = 0; i < Samples.Num(); i++)
    {
        if (Samples[i].Weight == 0) //|| i == MasterIndex)
            continue;
        Samples[i].SequenceNode->SetPlayRate(MaxPlayTime);
    }
}

void FAnimNode_BlendSpace1D::SynchronizeSampleTimes()
{
    if (!bIsTimeSynchronized) { return; }

    // 가장 Weight가 큰 샘플을 마스터로 선택
    int32 MasterIndex = INDEX_NONE;
    float MaximumWeight = 0.0f;

    for (int32 Index = 0; Index < Samples.Num(); ++Index)
    {
        if (Samples[Index].Weight > MaximumWeight)
        {
            MaximumWeight = Samples[Index].Weight;
            MasterIndex = Index;
        }
    }

    if (MasterIndex == INDEX_NONE) { return; }

    FBlendSample1D& MasterSample = Samples[MasterIndex];
    if (MasterSample.SequenceNode == nullptr) { return; }

    float MasterNormalizedTime = MasterSample.SequenceNode->GetNormalizedTime();

    // 나머지 시퀀스들 시간 맞춰주기
    for (int32 Index = 0; Index < Samples.Num(); ++Index)
    {
        if (Index == MasterIndex)
        {
            continue;
        }

        FBlendSample1D& Sample = Samples[Index];
        if (Sample.SequenceNode && Sample.Weight > KINDA_SMALL_NUMBER)
        {
            Sample.SequenceNode->SetNormalizedTime(MasterNormalizedTime);
        }
    }
}


// 주어진 값(InputValue)가 Grid 중 어느 Index 사이에 있는지+Weight 찾음
static void FindGridInterval(
    const TArray<float>& GridValues,
    float InputValue,
    int32& OutLowerIndex,
    int32& OutUpperIndex,
    float& OutWeight)
{
    const int32 Count = GridValues.Num();
    if (Count == 0)
    {
        OutLowerIndex = OutUpperIndex = 0; OutWeight = 0.0f; return;
    }

    if (InputValue <= GridValues[0])
    {
        OutLowerIndex = OutUpperIndex = 0; OutWeight = 0.0f; return;
    }
    
    if (InputValue >= GridValues[Count-1])
    {
        OutLowerIndex = OutUpperIndex = Count-1; OutWeight = 0.0f; return;
    }

    for (int32 Index = 0; Index < Count - 1; ++Index)
    {
        const float Value0 = GridValues[Index];
        const float Value1 = GridValues[Index+1];

        if (InputValue >= Value0 && InputValue <= Value1)
        {
            OutLowerIndex = Index;
            OutUpperIndex = Index + 1;

            const float Range = Value1 - Value0;
            OutWeight = (Range > KINDA_SMALL_NUMBER)
                ? (InputValue - Value0) / Range
                : 0.0f;

            return;
        }
    }
}

void FAnimNode_BlendSpace2D::Update(const FAnimationUpdateContext& Context)
{
    if (Samples.Num() == 0) { return; }

    CalculateSampleWeights();

    if (bIsTimeSynchronized)
    {
        SimpleSynchronizeSampleTimes();
        // SynchronizeSampleTimes();
    }

    for (FBlendSample2D& Sample : Samples)
    {
        if (Sample.Weight > KINDA_SMALL_NUMBER && Sample.SequenceNode)
        {
            Sample.SequenceNode->Update(Context);
        }
    }
}

void FAnimNode_BlendSpace2D::Evaluate(FPoseContext& Output)
{
    if (!Output.Skeleton) { return; }

    if (Samples.Num() == 0) { Output.ResetToRefPose(); return; }

    FPoseContext SamplePose(Output);
    const int32 NumBones = Output.EvaluatedPoses.Num();

    bool HasAnyPose = false;
    float TotalWeight = 0.0f;

    for (FBlendSample2D& Sample : Samples)
    {
        if (Sample.Weight <= KINDA_SMALL_NUMBER || Sample.SequenceNode == nullptr) { continue; }

        SamplePose.ResetToRefPose();
        Sample.SequenceNode->Evaluate(SamplePose);

        if (!HasAnyPose)
        {
            Output.EvaluatedPoses = SamplePose.EvaluatedPoses;
            TotalWeight = Sample.Weight;
            HasAnyPose = true;
        }
        else
        {
            const float NewTotalWeight = TotalWeight + Sample.Weight;
            const float Alpha = Sample.Weight / NewTotalWeight;

            for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
            {
                Output.EvaluatedPoses[BoneIndex] =
                    FTransform::Lerp(
                        Output.EvaluatedPoses[BoneIndex],
                        SamplePose.EvaluatedPoses[BoneIndex],
                        Alpha
                    );
            }
            TotalWeight = NewTotalWeight;
        }
    }
    if (!HasAnyPose) { Output.ResetToRefPose(); }
}

void FAnimNode_BlendSpace2D::CalculateSampleWeights()
{
    const int32 NumX = GridXValues.Num();
    const int32 NumY = GridYValues.Num();

    if (NumX == 0 || NumY == 0) { return; }

    // 1) 모든 Weight 초기화
    for (FBlendSample2D& Sample : Samples) { Sample.Weight = 0.0f; }

    // 2) X/Y 각각에서 구간 및 보간 인자 찾기
    int32 LowerXIndex = 0, UpperXIndex = 0; float XAlpha = 0;
    int32 LowerYIndex = 0, UpperYIndex = 0; float YAlpha = 0;

    FindGridInterval(GridXValues, BlendInputX, LowerXIndex, UpperXIndex, XAlpha);
    FindGridInterval(GridYValues, BlendInputY, LowerYIndex, UpperYIndex, YAlpha);

    // 3) 예외 케이스, 축이 한쪽으로 몰려서 Lower == Upper일 때
    const bool bSingleX = (LowerXIndex == UpperXIndex);
    const bool bSingleY = (LowerYIndex == UpperYIndex);

    const int32 NumGridX = NumX;

    auto GetSampleIndex = [NumGridX](int32 XIndex, int32 YIndex)
    {
        return XIndex + YIndex * NumGridX;
    };

    if (bSingleX && bSingleY)
    {
        // 딱 하나의 격자 포인트
        const int32 SampleIndex = GetSampleIndex(LowerXIndex, LowerYIndex);
        Samples[SampleIndex].Weight = 1.0f;
        return;
    }

    if (bSingleX && !bSingleY)
    {
        // 수직선 위에 있음 → 1D 보간 (Y축만)
        const int32 SampleIndexBottom = GetSampleIndex(LowerXIndex, LowerYIndex);
        const int32 SampleIndexTop = GetSampleIndex(LowerXIndex, UpperYIndex);

        Samples[SampleIndexBottom].Weight = 1.0f - YAlpha;
        Samples[SampleIndexTop].Weight = YAlpha;
        return;
    }

    if (!bSingleX && bSingleY)
    {
        // 수평선 위에 있음 → 1D 보간 (X축만)
        const int32 SampleIndexLeft = GetSampleIndex(LowerXIndex, LowerYIndex);
        const int32 SampleIndexRight = GetSampleIndex(UpperXIndex, LowerYIndex);

        Samples[SampleIndexLeft].Weight = 1.0f - XAlpha;
        Samples[SampleIndexRight].Weight = XAlpha;
        return;
    }
    
    // 4) 일반 케이스: 사각형 셀 안에 있음 → bilinear interpolation
    const int32 SampleIndexBottomLeft = GetSampleIndex(LowerXIndex, LowerYIndex);
    const int32 SampleIndexBottomRight = GetSampleIndex(UpperXIndex, LowerYIndex);
    const int32 SampleIndexTopLeft = GetSampleIndex(LowerXIndex, UpperYIndex);
    const int32 SampleIndexTopRight = GetSampleIndex(UpperXIndex, UpperYIndex);

    const float WeightBottomLeft = (1.0f - XAlpha) * (1.0f - YAlpha);
    const float WeightBottomRight = XAlpha * (1.0f - YAlpha);
    const float WeightTopLeft = (1.0f - XAlpha) * YAlpha;
    const float WeightTopRight = XAlpha * YAlpha;

    Samples[SampleIndexBottomLeft].Weight = WeightBottomLeft;
    Samples[SampleIndexBottomRight].Weight = WeightBottomRight;
    Samples[SampleIndexTopLeft].Weight = WeightTopLeft;
    Samples[SampleIndexTopRight].Weight = WeightTopRight;
}

void FAnimNode_BlendSpace2D::SimpleSynchronizeSampleTimes()
{
    if (bIsTimeSynchronized) { return; }
    bIsTimeSynchronized = true;

    float MaxPlayTime = 0;
    int32 MasterIndex = 0;
    for (int i = 0; i < Samples.Num(); i++)
    {
        float PlayTime = Samples[i].SequenceNode->GetLength();
        if (PlayTime > MaxPlayTime)
        {
            MaxPlayTime = PlayTime;
            MasterIndex = i;
        }
    }

    for (int i = 0; i < Samples.Num(); i++)
    {
        if (Samples[i].Weight == 0) //|| i == MasterIndex)
            continue;
        Samples[i].SequenceNode->SetPlayRate(MaxPlayTime);
    }
}

void FAnimNode_BlendSpace2D::SynchronizeSampleTimes()
{
    if (!bIsTimeSynchronized) { return; }

    // 가장 Weight가 큰 샘플을 마스터로 선택
    int32 MasterIndex = INDEX_NONE;
    float MaximumWeight = 0.0f;

    for (int32 Index = 0; Index < Samples.Num(); ++Index)
    {
        if (Samples[Index].Weight > MaximumWeight)
        {
            MaximumWeight = Samples[Index].Weight;
            MasterIndex = Index;
        }
    }

    if (MasterIndex == INDEX_NONE) { return; }

    FBlendSample2D& MasterSample = Samples[MasterIndex];
    if (MasterSample.SequenceNode == nullptr) { return; }

    float MasterNormalizedTime = MasterSample.SequenceNode->GetNormalizedTime();

    // 나머지 시퀀스들 시간 맞춰주기
    for (int32 Index = 0; Index < Samples.Num(); ++Index)
    {
        if (Index == MasterIndex)
        {
            continue;
        }

        FBlendSample2D& Sample = Samples[Index];
        if (Sample.SequenceNode && Sample.Weight > KINDA_SMALL_NUMBER)
        {
            Sample.SequenceNode->SetNormalizedTime(MasterNormalizedTime);
        }
    }
}void FAnimNode_BlendSpace1D::UpdateRangeFromSamples()
{
    if (Samples.Num() == 0)
    {
        if (!bHasManualMin)
        {
            MinimumPosition = 0.0f;
        }

        if (!bHasManualMax)
        {
            MaximumPosition = 1.0f;
        }
        return;
    }

    if (!bHasManualMin)
    {
        MinimumPosition = Samples[0].Position;
    }

    if (!bHasManualMax)
    {
        MaximumPosition = Samples.Last().Position;
    }
}
