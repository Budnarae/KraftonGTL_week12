#pragma once

#include "AnimInstance.h"

class UAnimSingleNodeInstance : public UAnimInstance
{
    DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)

    virtual ~UAnimSingleNodeInstance();
    
    /**
     * @brief 재생할 애니메이션을 설정합니다 (시간 초기화)
     * @param NewAnimation 설정할 애니메이션 에셋
     */
    void SetAnimation(class UAnimationAsset* NewAnimation);

    /**
     * @brief 애니메이션을 재생합니다 (편의 함수)
     * @param NewAnimToPlay 재생할 애니메이션 에셋
     * @param bLooping 루프 재생 여부
     */    
    void PlayAnimation(class UAnimationAsset* NewAnimToPlay, bool bLooping = true);

    // ====================================
    // State Query
    // ====================================
    /**
     * @brief 현재 재생 중인 애니메이션 반환
     */
    UAnimationAsset* GetCurrentAnimation() const { return CurrentAnimation; }

    void NativeUpdateAnimation(float DeltaSeconds) override;
    void EvaluateAnimation() override;
private:
    UAnimationAsset* CurrentAnimation = nullptr;        // 현재 재생 중인 애니메이션
};