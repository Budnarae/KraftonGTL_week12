#pragma once
#include "AnimationAsset.h"
#include "AnimDataModel.h"

class UAnimationSequence : public UAnimationAsset
{
    DECLARE_CLASS(UAnimationSequence, UAnimationAsset)
public:
    UAnimationSequence();
    virtual ~UAnimationSequence() override;

    void Load(const FString& InFilePath, ID3D11Device* InDevice) override;
    void Save(const FString& InFilePath) override;

    // 애니메이션 재생
    FTransform GetBonePose(const FName& BoneName, float Time) const;

    // DataModel 접근
    UAnimDataModel* GetDataModel() const { return DataModel; }

    // Getter
    const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const;
    float GetPlayLength() const;
    float GetFrameRate() const;
    int32 GetNumberOfFrames() const;
    int32 GetNumberOfKeys() const;

private:
    UAnimDataModel* DataModel = nullptr;
};
