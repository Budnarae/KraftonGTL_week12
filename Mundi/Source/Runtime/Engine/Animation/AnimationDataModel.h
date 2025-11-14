#pragma once

UCLASS()
class UAnimationDataModel : public UResourceBase
{
    DECLARE_CLASS(UAnimationDataModel, UResourceBase)
public:
    UAnimationDataModel();
    virtual ~UAnimationDataModel() override;

    void Load(const FString& InFilePath, ID3D11Device* InDevice);
    
    virtual const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const {return BoneAnimationTracks;}
    float GetPlayLength() const { return PlayLength; }
    float GetFrameRate() const { return FrameRate; }
    int32 GetNumberOfFrames() const { return NumberOfFrames; }
    int32 GetNumberOfKeys() const { return NumberOfKeys; }

    void Initialize(const TArray<FBoneAnimationTrack>& InBoneAnimationTracks,
                            float InPlayLength,
                            float InFrameRate)
    {
        BoneAnimationTracks = InBoneAnimationTracks;
        PlayLength = InPlayLength;
        FrameRate = InFrameRate;
        NumberOfFrames = FMath::RoundToInt(PlayLength * FrameRate);

        NumberOfKeys = 0;
        for (const FBoneAnimationTrack& Track : BoneAnimationTracks)
        {
            NumberOfKeys += Track.InternalTrack.PosKeys.Num();
        }
    }
    
private:
    TArray<FBoneAnimationTrack> BoneAnimationTracks{};
    float PlayLength{};
    float FrameRate{};
    int32 NumberOfFrames{};
    int32 NumberOfKeys{};
    //FAnimationCurveData CurveData{};
};
