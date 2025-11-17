#pragma once
#include "AnimNodeTransitionRule.h"
#include "AnimNode.h"

class USkeletalMeshComponent;
class UAnimationSequence;
class UAnimInstance : public UObject
{
public:
    DECLARE_CLASS(UAnimInstance, UObject)
    
    virtual ~UAnimInstance();

    void SetSkeletalComponent(USkeletalMeshComponent* InSkeletalMeshComponent);
    USkeletalMeshComponent* GetSkeletalComponent() const { return OwnerSkeletalComp; }

    // C++ AnimationStateMachine 사용 (현재 주석 처리 - Lua ASM 사용)
    // FAnimNode_StateMachine ASM;
    // FAnimNode_Base* RootNode = nullptr;

    // 현재 포즈를 저장할 변수
    FPoseContext           CurrentPose;

    float CurTime = 0.0;

    /**
     * @brief AnimInstance 초기화 (최초 1회 호출)
     */
    void Initialize();
    
    /**
     * @brief 매 프레임 애니메이션 시간을 업데이트하고 본 포즈를 갱신합니다
     * @param DeltaTime 프레임 델타 타임
     */
    void UpdateAnimation(float DeltaTime);

protected:
    /* Unreal Style */

    // - 게임 로직 기준 Update (Tick) 단계
    // - 파라미터 업데이트, Transition 조건에 필요한 변수 갱신
    virtual void NativeUpdateAnimation(float DeltaSeconds);

    // - AnimGraph Evaluate 단계 수행
    // - 최종 Pose 계산
    virtual void EvaluateAnimation();

public:
    FPoseContext& GetCurrentPose() { return CurrentPose; };

// ====================================
// C++ ASM 관련 코드 (주석 처리 - Lua ASM 사용)
// ====================================
// public:
//     void AddTransitionRule(UAnimNodeTransitionRule* InRule);
//     void RemoveTransitionRule(const FName& RuleName);
//     UAnimNodeTransitionRule* FindTransitionRuleByName(const FName& RuleName) const;
//     const TArray<UAnimNodeTransitionRule*>& GetAllTransitionRules() const { return TransitionRules; }
//     void InitializeAnimationStateMachine();

protected:
    USkeletalMeshComponent* OwnerSkeletalComp = nullptr;

    // C++ ASM 관련 변수 (주석 처리 - Lua ASM 사용)
    // TArray<UAnimNodeTransitionRule*> TransitionRules;
    // float TransitionTimer = 0.0f;
    // const float TransitionInterval = 10.0f;
    // FAnimState* PreviousState = nullptr;

private:
    bool bIsInitialized = false;
};

