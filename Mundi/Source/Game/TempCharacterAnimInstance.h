#pragma once
#include "AnimInstance.h"

struct FAnimNode_Sequence;
struct FAnimNode_BlendSpace1D;
class UAnimationSequence;

/**
 * TempCharacter용 AnimInstance
 * 이동 속도에 따라 Idle/Walk/Run 애니메이션을 블렌딩합니다.
 */
class UTempCharacterAnimInstance : public UAnimInstance
{
public:
    DECLARE_CLASS(UTempCharacterAnimInstance, UAnimInstance)

    virtual ~UTempCharacterAnimInstance();

    /**
     * @brief 이동 속도 설정 (0~600 범위 권장)
     * @param InSpeed 현재 이동 속도
     */
    void SetSpeed(float InSpeed);

    /**
     * @brief 현재 이동 속도 반환
     */
    float GetSpeed() const { return CurrentSpeed; }

protected:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual void EvaluateAnimation() override;

private:
    /**
     * @brief 애니메이션 State Machine 초기화
     */
    void InitializeAnimationStateMachine();

private:
    // 현재 이동 속도
    float CurrentSpeed = 0.0f;

    // 애니메이션 노드들
    FAnimNode_Sequence* IdleSequenceNode = nullptr;
    FAnimNode_Sequence* WalkSequenceNode = nullptr;
    FAnimNode_Sequence* RunSequenceNode = nullptr;
    FAnimNode_BlendSpace1D* MoveBlendSpaceNode = nullptr;

    // 초기화 여부
    bool bAnimationInitialized = false;
};
