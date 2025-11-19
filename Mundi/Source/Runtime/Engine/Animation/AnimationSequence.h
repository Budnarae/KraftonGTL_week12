#pragma once
#include "AnimationAsset.h"
#include "AnimDataModel.h"
#include "AnimationType.h"

class UAnimNotify;
class UAnimNotifyState;

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
    bool Save(const FString& InFilePath = "") override;

    FTransform GetBonePose(const FName& BoneName, float Time) const override;
    float GetPlayLength() const override;
    float GetFrameRate() const override;

    virtual void Update(const FAnimationUpdateContext& Context) override;
    virtual void Evaluate(FPoseContext& Output) override;

    // ====================================
    // AnimSequence 전용
    // ====================================

    UAnimDataModel* GetDataModel() const { return DataModel; }
    void SetDataModel(UAnimDataModel* InDataModel) { DataModel = InDataModel; }

    void SetSkeleton(const FSkeleton& InSkeleton) { Skeleton = InSkeleton; }

    void SetLooping(bool bInLooping) { bIsLooping = bInLooping; }
    bool IsLooping() const { return bIsLooping; }

    void ResetPlaybackTime() { CurrentAnimationTime = 0.0f; }

    void SetCurrentTime(float InTime) { CurrentAnimationTime = InTime; }
    float GetCurrentTime() const { return CurrentAnimationTime; }

    const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const;

    void EvaluatePose(float Time, FPoseContext& Out) const;

    int32 GetNumberOfFrames() const;
    int32 GetNumberOfKeys() const;

    // ====================================
    // AnimNotify 관리
    // ====================================

    void AddAnimNotify(UAnimNotify* InNotify);
    void RemoveAnimNotify(UAnimNotify* InNotify);
    void ClearAnimNotifies() { AnimNotifies.Empty(); }
    const TArray<UAnimNotify*>& GetAnimNotifies() const { return AnimNotifies; }
    int32 GetAnimNotifyCount() const { return AnimNotifies.Num(); }

    // ====================================
    // AnimNotifyState 관리
    // ====================================

    void AddAnimNotifyState(UAnimNotifyState* InNotifyState);
    void RemoveAnimNotifyState(UAnimNotifyState* InNotifyState);
    void ClearAnimNotifyStates() { AnimNotifyStates.Empty(); }
    const TArray<UAnimNotifyState*>& GetAnimNotifyStates() const { return AnimNotifyStates; }
    int32 GetAnimNotifyStateCount() const { return AnimNotifyStates.Num(); }
private:
    UAnimDataModel* DataModel = nullptr;
    FSkeleton Skeleton{};
    float CurrentAnimationTime = 0.0f;
    bool bIsLooping = true;

    // AnimNotify 목록 (이 Sequence에 속한 Notify들)
    TArray<UAnimNotify*> AnimNotifies;

    // AnimNotifyState 목록
    TArray<UAnimNotifyState*> AnimNotifyStates;
};



