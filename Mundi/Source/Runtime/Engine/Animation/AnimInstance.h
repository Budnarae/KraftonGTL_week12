#pragma once

class USkeletalMeshComponent;
class UAnimationSequence;
class UAnimInstance : public UObject
{
public:
    DECLARE_CLASS(UAnimInstance, UObject)
    
    virtual ~UAnimInstance();

    void SetSkeletalComponent(USkeletalMeshComponent* InSkeletalMeshComponent) { OwnerSkeletalComp = InSkeletalMeshComponent; }
    USkeletalMeshComponent* GetSkeletalComponent() const { return OwnerSkeletalComp; }

    // FOR TEST
    void PlayBlendedAnimation(UAnimationSequence& InSeqA, UAnimationSequence& InSeqB);
    void UpdateBlendedAnimation(float DeltaTime, float Alpha);

    UAnimationSequence* TestSeqA = nullptr;
    UAnimationSequence* TestSeqB = nullptr;
    bool IsBlending = false;
    float CurTime = 0.0;
    
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
    
// FOR TEST!!!
private:
    float TestTime = 0;
    bool bIsInitialized = false;
    FTransform TestBoneBasePose;
};

