#pragma once
#include "AnimationAsset.h"
#include "AnimDataModel.h"

class UAnimationSequence : public UAnimationAsset
{
    DECLARE_CLASS(UAnimationSequence, UAnimationAsset)
public:
    UAnimationSequence();
    UAnimationSequence(const FSkeleton& InSkeleton);
    virtual ~UAnimationSequence() override;

    // ====================================
    // UAnimationAsset 인터페이스 구현
    // ====================================

    void Load(const FString& InFilePath, ID3D11Device* InDevice) override;
    void Save(const FString& InFilePath) override;

    FTransform GetBonePose(const FName& BoneName, float Time) const override;
    float GetPlayLength() const override;
    float GetFrameRate() const override;

    // ====================================
    // AnimSequence 전용
    // ====================================

    /**
     * @brief 내부 DataModel에 접근합니다
     */
    UAnimDataModel* GetDataModel() const { return DataModel; }

    /**
     * @brief DataModel을 설정합니다
     */
    void SetDataModel(UAnimDataModel* InDataModel) { DataModel = InDataModel; }

    /**
     * @brief Skeleton을 설정합니다
     */
    void SetSkeleton(const FSkeleton& InSkeleton) { Skeleton = InSkeleton; }

    /**
     * @brief 루핑 여부를 설정합니다
     */
    void SetLooping(bool InLooping) { bIsLooping = InLooping; }

    /**
     * @brief 모든 본의 애니메이션 트랙을 반환합니다
     */
    const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const;

    /**
     * @brief 총 프레임 수를 반환합니다
     */
    int32 GetNumberOfFrames() const;

    /**
     * @brief 총 키프레임 수를 반환합니다
     */
    int32 GetNumberOfKeys() const;

    void Update(const FAnimationUpdateContext& Context);
    void Evaluate(FPoseContext& Output);

private:
    UAnimDataModel* DataModel = nullptr;
    FSkeleton Skeleton{};

    /* 재생 관련 */
    float CurrentAnimationTime = 0.0f;  // 현재 애니메이션 재생 시간
    bool bIsLooping = false;
};
