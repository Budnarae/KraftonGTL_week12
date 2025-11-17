#pragma once

#include <sol/sol.hpp>

struct FRawAnimSequenceTrack
{
    TArray<FVector> PosKeys; // 위치 키프레임
    TArray<FVector4>   RotKeys; // 회전 키프레임 (Quaternion)
    TArray<FVector> ScaleKeys; // 스케일 키프레임

    friend FArchive& operator<<(FArchive& Ar, FRawAnimSequenceTrack& Track)
    {
        if (Ar.IsSaving())
        {
            Serialization::WriteArray(Ar, Track.PosKeys);
            Serialization::WriteArray(Ar, Track.RotKeys);
            Serialization::WriteArray(Ar, Track.ScaleKeys);
        }
        else if (Ar.IsLoading())
        {
            Serialization::ReadArray(Ar, Track.PosKeys);
            Serialization::ReadArray(Ar, Track.RotKeys);
            Serialization::ReadArray(Ar, Track.ScaleKeys);
        }
        return Ar;
    }
};

struct FBoneAnimationTrack
{
    FName Name;                        // Bone 이름
    FRawAnimSequenceTrack InternalTrack; // 실제 애니메이션 데이터

    friend FArchive& operator<<(FArchive& Ar, FBoneAnimationTrack& Track)
    {
        if (Ar.IsSaving())
        {
            FString NameStr = Track.Name.ToString();
            Serialization::WriteString(Ar, NameStr);
            Ar << Track.InternalTrack;
        }
        else if (Ar.IsLoading())
        {
            FString NameStr;
            Serialization::ReadString(Ar, NameStr);
            Track.Name = FName(NameStr);
            Ar << Track.InternalTrack;
        }
        return Ar;
    }
};

namespace Serialization {
    template<>
    inline void WriteArray<FBoneAnimationTrack>(FArchive& Ar, const TArray<FBoneAnimationTrack>& Arr) {
        uint32 Count = (uint32)Arr.size();
        Ar << Count;
        for (auto& Track : Arr) Ar << const_cast<FBoneAnimationTrack&>(Track);
    }

    template<>
    inline void ReadArray<FBoneAnimationTrack>(FArchive& Ar, TArray<FBoneAnimationTrack>& Arr) {
        uint32 Count;
        Ar << Count;
        Arr.resize(Count);
        for (auto& Track : Arr) Ar << Track;
    }
}

struct FAnimationUpdateContext
{
    // ------------------------------------------------------------------------
    // DeltaTime
    // 현재 Tick 동안 경과한 시간 (초)
    // 모든 AnimNode Update에서 참조
    // ------------------------------------------------------------------------
    float DeltaTime{};

    // ------------------------------------------------------------------------
    // bIsSkeletonInitialised
    // SkeletalMesh가 초기화되었는지 여부
    // Update 수행 전 체크
    // ------------------------------------------------------------------------
    // bool bIsSkeletonInitialized;

    // ------------------------------------------------------------------------
    // bEnableRootMotion
    // RootMotion 적용 여부
    // ------------------------------------------------------------------------
    // bool bEnableRootMotion;

    /* 추후 필요한 정보를 추가 */
};

struct FPoseContext
{
    const FSkeleton* Skeleton = nullptr;
    TArray<FTransform> EvaluatedPoses;

    FPoseContext() = default;

    FPoseContext(const FSkeleton* InSkeleton)
        : Skeleton(InSkeleton)
    {
        if (Skeleton)
        {
            EvaluatedPoses.SetNum(Skeleton->Bones.Num());
        }
    }
};

// 전방 선언
class UAnimNodeTransitionRule;

// Forward declaration
struct FAnimState;

struct FAnimStateTransition
{
    // ------------------------------------------------------------------------
    // Source / Target State
    // Transition 출발 상태와 도착 상태 포인터
    // ------------------------------------------------------------------------
    FAnimState* SourceState = nullptr;
    FAnimState* TargetState = nullptr;

    uint32 Index{};   // Animation State Machine에서의 Index

    // ------------------------------------------------------------------------
    // Transition 조건
    // true가 되면 Transition 발동
    // ------------------------------------------------------------------------
    bool CanEnterTransition = false;

    // Lua 함수 기반 Transition 조건
    // Lua에서 조건 함수를 설정하고, Update 시 호출하여 CanEnterTransition 업데이트
    sol::function TransitionConditionFunc;

    // 생성자/소멸자
    FAnimStateTransition() = default;
    FAnimStateTransition(const FAnimStateTransition& Other);
    FAnimStateTransition& operator=(const FAnimStateTransition& Other);
    ~FAnimStateTransition();

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
    
    /* 이하는 나중에 해제하여 사용할 것 */
    
    // ------------------------------------------------------------------------
    // Transition 블렌딩 시간
    // ActiveState Pose -> TargetState Pose로 자연스럽게 Blend
    // ------------------------------------------------------------------------
    float BlendTime = 0.2f;

    // ------------------------------------------------------------------------
    // Transition 블렌딩 경과 시간
    // 경과 시간이 지나면 다음 State로 전환
    // ------------------------------------------------------------------------
    float BlendTimeElapsed = 0.f;

    // ------------------------------------------------------------------------
    // Transition 진행 상태
    // BlendAlpha: 0.0 = SourcePose, 1.0 = TargetPose
    // ------------------------------------------------------------------------
    float BlendAlpha = 0.0f;

    bool bIsBlending = false;

    // ------------------------------------------------------------------------
    // Interrupt 옵션
    // 다른 Transition으로 중단 가능한지 여부
    // ------------------------------------------------------------------------
    //bool bCanInterrupt = true;

    // ------------------------------------------------------------------------
    // Blending 시작 시 파라미터 세팅
    // ------------------------------------------------------------------------
    void StartBlending()
    {
        bIsBlending = true;
        BlendTimeElapsed = 0.f;
        BlendAlpha = 0.f;
    }
    
    // TODO: Blending Helper 함수 구현 이후 Update Evaluate 구현
    // ------------------------------------------------------------------------
    // Update
    // DeltaTime 기반으로 BlendAlpha 계산
    // Condition 평가 후 Blend 진행
    // ------------------------------------------------------------------------
    void Update(const FAnimationUpdateContext& Context)
    {
        // Transition 조건 평가 (Lua 함수 호출)
        if (TransitionConditionFunc.valid())
        {
            CanEnterTransition = EvaluateCondition();
        }

        // Blending 업데이트
        if (bIsBlending)
        {
            BlendTimeElapsed += Context.DeltaTime;
            if (BlendTimeElapsed >= BlendTime)
            {
                bIsBlending = false;
                return;
            }

            // 블렌딩은 추후 구현
        }
    }

    // ------------------------------------------------------------------------
    // Evaluate
    // SourcePose와 TargetPose를 BlendAlpha 기준으로 보간
    // ------------------------------------------------------------------------
    void Evaluate(FPoseContext& Output)
    {
        // 블렌딩은 추후 구현
    }

    // ------------------------------------------------------------------------
    // Transition Condition Converter
    // 특정 조건을 충족하면 외부의 delegate에서 호출
    // ------------------------------------------------------------------------
    void TriggerTransition()
    {
        CanEnterTransition = true;
    }

    // ------------------------------------------------------------------------
    // Setter for BlendTime
    // ------------------------------------------------------------------------
    void SetBlendTime(float InBlendTime)
    {
        BlendTime = InBlendTime;
    }
};