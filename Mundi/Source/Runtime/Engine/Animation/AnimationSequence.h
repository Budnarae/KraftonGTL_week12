#pragma once
#include "AnimationAsset.h"
#include "AnimDataModel.h"

class UAnimationSequence : public UAnimationAsset
{
    DECLARE_CLASS(UAnimationSequence, UAnimationAsset)
public:
    UAnimationSequence();
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
     * @brief 모든 본의 애니메이션 트랙을 반환합니다
     */
    const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const;

    void EvaluatePose(float Time, const FSkeleton& Skeleton, FPoseContext& Out) const;

    /**
     * @brief 총 프레임 수를 반환합니다
     */
    int32 GetNumberOfFrames() const;

    /**
     * @brief 총 키프레임 수를 반환합니다
     */
    int32 GetNumberOfKeys() const;

private:
    UAnimDataModel* DataModel = nullptr;
};
