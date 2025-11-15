#pragma once

#include "AnimationStateMachine.h"
#include "AnimNodeTransitionRule.h"

class USkeletalMeshComponent;
class UAnimInstance : public UObject
{
public:
    DECLARE_CLASS(UAnimInstance, UObject)
    
    void SetSkeletalComponent(USkeletalMeshComponent* InSkeletalMeshComponent) { OwnerSkeletalComp = InSkeletalMeshComponent; }
    USkeletalMeshComponent* GetSkeletalComponent() const { return OwnerSkeletalComp; }
    
    /**
     * @brief 매 프레임 애니메이션 시간을 업데이트하고 본 포즈를 갱신합니다
     * @param DeltaTime 프레임 델타 타임
     */
    void UpdateAnimation(float DeltaTime);
    
    /**
     * @brief 재생할 애니메이션을 설정합니다 (시간 초기화)
     * @param NewAnimation 설정할 애니메이션 에셋
     */
    void SetAnimation(class UAnimationAsset* NewAnimation);

    /**
     * @brief 현재 설정된 애니메이션을 재생 시작합니다
     * @param bLooping 루프 재생 여부
     */
    void Play(bool bLooping = true);
    
    /**
     * @brief 애니메이션을 재생합니다 (편의 함수)
     * @param NewAnimToPlay 재생할 애니메이션 에셋
     * @param bLooping 루프 재생 여부
     */    
    void PlayAnimation(class UAnimationAsset* NewAnimToPlay, bool bLooping = true);

    /**
     * @brief 현재 재생 중인 애니메이션을 정지하고 BindPose로 복원합니다
     */
    void StopAnimation();

    /* Unreal Style */
    
    // - 게임 로직 기준 Update (Tick) 단계
    // - 파라미터 업데이트, Transition 조건에 필요한 변수 갱신
    //
    // 원본 Unreal에서는 virtual로 되어있음.
    // 아마 추후 SingleNodeInstance 구현과 관련이 있는 듯 함
    void NativeUpdateAnimation(float DeltaSeconds);

    // - AnimGraph Evaluate 단계 수행
    // - 최종 Pose 계산
    void EvaluateAnimation();

    FPoseContext& GetCurrentPose();

// ====================================
// Transition Rule Management
// ====================================
public:
    /**
     * @brief Transition Rule 추가
     * @param InRule 추가할 Rule 객체
     */
    void AddTransitionRule(UAnimNodeTransitionRule* InRule);

    /**
     * @brief 이름으로 Transition Rule 제거
     * @param RuleName 제거할 Rule 이름
     */
    void RemoveTransitionRule(const FName& RuleName);

    /**
     * @brief 이름으로 Transition Rule 찾기
     * @param RuleName 찾을 Rule 이름
     * @return Rule 객체, 없으면 nullptr
     */
    UAnimNodeTransitionRule* FindTransitionRuleByName(const FName& RuleName) const;

    /**
     * @brief 모든 Transition Rule 반환
     */
    const TArray<UAnimNodeTransitionRule*>& GetAllTransitionRules() const { return TransitionRules; }

// ====================================
// Animation State Machine Setup
// ====================================
public:
    /**
     * @brief Animation State Machine 초기화 (테스트용)
     */
    void InitializeAnimationStateMachine();

    /**
     * @brief Animation State Machine 접근자
     */
    UAnimationStateMachine* GetAnimationStateMachine() { return &ASM; }

// ====================================
// State Query
// ====================================
public:
    /**
     * @brief 현재 재생 중인 애니메이션 반환
     */
    UAnimationAsset* GetCurrentAnimation() const { return CurrentAnimation; }
    
    /**
     * @brief 현재 애니메이션 재생 시간 반환
     */
    float GetCurrentAnimationTime() const { return CurrentAnimationTime; }
    
    /**
     * @brief 재생 중인지 여부 반환
     */
    bool IsPlaying() const { return bIsPlaying; }
    
private:
    USkeletalMeshComponent* OwnerSkeletalComp = nullptr;
    UAnimationAsset* CurrentAnimation = nullptr;        // 현재 재생 중인 애니메이션

    float CurrentAnimationTime = 0.0f;                  // 현재 애니메이션 재생 시간 (초)
    bool bIsPlaying = false;                            // 재생 중 여부
    bool bIsLooping = true;                             // 루프 재생 여부

    // 현재 포즈를 저장할 변수
    FPoseContext CurrentPose;
    UAnimationStateMachine ASM;

    // Transition Rule 관리
    TArray<UAnimNodeTransitionRule*> TransitionRules;

    // 10초마다 애니메이션 전환 타이머
    float TransitionTimer = 0.0f;
    const float TransitionInterval = 10.0f;

// FOR TEST!!!
private:
    float TestTime = 0;
    bool bIsInitialized = false;
    FTransform TestBoneBasePose;
};

