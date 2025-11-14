#include "pch.h"
#include "AnimationSequence.h"

IMPLEMENT_CLASS(UAnimationSequence)

UAnimationSequence::UAnimationSequence()
{
}

UAnimationSequence::~UAnimationSequence()
{
    if (DataModel)
    {
        DataModel = nullptr;
    }
}

void UAnimationSequence::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
    // FBX Loader에서 FBX 로드 시 한번에 처리하므로 실제 사용은 안할 듯
    // FBX Loader에서 TArray<FBoneAnimationTrack> 생성 => Data Model 생성 -> Sequence 생성
    // FBX Loader에서는 ResourceManager::ADD 메서드를 통해 리소스 매니저에 책임 전달
}

void UAnimationSequence::Save(const FString& InFilePath)
{
    
}

FTransform UAnimationSequence::GetBonePose(const FName& BoneName, float Time) const
{
    if (!DataModel)
        return FTransform();

    const FBoneAnimationTrack* AnimTrack = DataModel->FindTrackByBone(BoneName);
    if (!AnimTrack)
        return FTransform();

    const FRawAnimSequenceTrack& Track = AnimTrack->InternalTrack;

    // 시간을 프레임 번호로 변환
    float FrameRate = DataModel->GetFrameRate();
    float PlayLength = DataModel->GetPlayLength();

    // 시간 클램프
    Time = FMath::Clamp(Time, 0.0f, PlayLength);
    float FrameTime = Time * FrameRate;

    // 프레임 인덱스 계산
    int32 FrameIndex0 = FMath::FloorToInt(FrameTime);
    int32 FrameIndex1 = FMath::CeilToInt(FrameTime);
    float Alpha = FrameTime - FrameIndex0;  // 보간 비율

    FTransform Result;

    // Position 보간 (Linear)
    if (Track.PosKeys.Num() > 0)
    {
        int32 PosIdx0 = FMath::Clamp(FrameIndex0, 0, Track.PosKeys.Num() - 1);
        int32 PosIdx1 = FMath::Clamp(FrameIndex1, 0, Track.PosKeys.Num() - 1);

        const FVector& Pos0 = Track.PosKeys[PosIdx0];
        const FVector& Pos1 = Track.PosKeys[PosIdx1];

        Result.Translation = FVector::Lerp(Pos0, Pos1, Alpha);
    }

    // Rotation 보간 (Slerp)
    if (Track.RotKeys.Num() > 0)
    {
        int32 RotIdx0 = FMath::Clamp(FrameIndex0, 0, Track.RotKeys.Num() - 1);
        int32 RotIdx1 = FMath::Clamp(FrameIndex1, 0, Track.RotKeys.Num() - 1);

        const FVector4& Rot0Vec = Track.RotKeys[RotIdx0];
        const FVector4& Rot1Vec = Track.RotKeys[RotIdx1];

        // FVector4 -> FQuat 변환
        FQuat Rot0(Rot0Vec.X, Rot0Vec.Y, Rot0Vec.Z, Rot0Vec.W);
        FQuat Rot1(Rot1Vec.X, Rot1Vec.Y, Rot1Vec.Z, Rot1Vec.W);

        Result.Rotation = FQuat::Slerp(Rot0, Rot1, Alpha);
        Result.Rotation.Normalize();
    }

    // Scale 보간 (Linear)
    if (Track.ScaleKeys.Num() > 0)
    {
        int32 ScaleIdx0 = FMath::Clamp(FrameIndex0, 0, Track.ScaleKeys.Num() - 1);
        int32 ScaleIdx1 = FMath::Clamp(FrameIndex1, 0, Track.ScaleKeys.Num() - 1);

        const FVector& Scale0 = Track.ScaleKeys[ScaleIdx0];
        const FVector& Scale1 = Track.ScaleKeys[ScaleIdx1];

        Result.Scale3D = FVector::Lerp(Scale0, Scale1, Alpha);
    }
    else
    {
        Result.Scale3D = FVector(1.0f, 1.0f, 1.0f);
    }

    return Result;
}

const TArray<FBoneAnimationTrack>& UAnimationSequence::GetBoneAnimationTracks() const
{
    static TArray<FBoneAnimationTrack> Empty;
    return DataModel ? DataModel->GetBoneAnimationTracks() : Empty;
}

float UAnimationSequence::GetPlayLength() const
{
    return DataModel ? DataModel->GetPlayLength() : 0.0f;
}

float UAnimationSequence::GetFrameRate() const
{
    return DataModel ? DataModel->GetFrameRate() : 0.0f;
}

int32 UAnimationSequence::GetNumberOfFrames() const
{
    return DataModel ? DataModel->GetNumberOfFrames() : 0;
}

int32 UAnimationSequence::GetNumberOfKeys() const
{
    return DataModel ? DataModel->GetNumberOfKeys() : 0;
}
