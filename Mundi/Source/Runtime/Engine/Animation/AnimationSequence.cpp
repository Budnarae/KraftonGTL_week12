#include "pch.h"
#include "AnimationSequence.h"
#include "WindowsBinWriter.h"
#include "AnimNotify/AnimNotify.h"
#include <filesystem>

IMPLEMENT_CLASS(UAnimationSequence)

UAnimationSequence::UAnimationSequence() {}
UAnimationSequence::UAnimationSequence(const FSkeleton& InSkeleton) : Skeleton(InSkeleton) {}

UAnimationSequence::~UAnimationSequence()
{
    // AnimNotify 정리
    for (UAnimNotify* Notify : AnimNotifies)
    {
        if (Notify)
        {
            DeleteObject(Notify);
        }
    }
    AnimNotifies.Empty();

    if (DataModel)
    {
        DeleteObject(DataModel);
        DataModel = nullptr;
    }
}

void UAnimationSequence::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
    // FBX Loader에서 .uanim 파일 로드를 담당하므로 여기서는 구현하지 않음
    // FBXLoader::LoadCachedAnimations()에서 .uanim 파일 읽기 후 UAnimationSequence 생성
}

bool UAnimationSequence::Save(const FString& InFilePath)
{
    if (!DataModel)
    {
        UE_LOG("AnimationSequence::Save failed: DataModel is null");
        return false;
    }

    FString SavePath = InFilePath.empty() ? FilePath : InFilePath;
    if (SavePath.empty())
    {
        UE_LOG("AnimationSequence::Save failed: No file path specified");
        return false;
    }

    // 디렉토리 생성
    std::filesystem::path FilePath(SavePath);
    if (FilePath.has_parent_path())
    {
        std::filesystem::create_directories(FilePath.parent_path());
    }

    try
    {
        FWindowsBinWriter Writer(SavePath);

        // 애니메이션 데이터 (FBXLoader와 동일한 형식)
        float PlayLength = DataModel->GetPlayLength();
        float FrameRate = DataModel->GetFrameRate();
        const TArray<FBoneAnimationTrack>& BoneTracks = DataModel->GetBoneAnimationTracks();

        Writer << PlayLength;
        Writer << FrameRate;
        Serialization::WriteArray(Writer, BoneTracks);

        Writer.Close();

        UE_LOG("AnimationSequence saved: %s (%.2fs, %.2f fps, %d tracks)",
               SavePath.c_str(), PlayLength, FrameRate, BoneTracks.Num());
        return true;
    }
    catch (const std::exception& e)
    {
        UE_LOG("AnimationSequence::Save failed: %s", e.what());
        return false;
    }
}

void UAnimationSequence::Update(const FAnimationUpdateContext& Context)
{
    if (!DataModel)
    {
        return;
    }

    const float PlayLength = GetPlayLength();
    if (PlayLength <= 0.0f)
    {
        return;
    }

    CurrentAnimationTime += Context.DeltaTime;

    if (bIsLooping)
    {
        CurrentAnimationTime = std::fmod(CurrentAnimationTime, PlayLength);
        if (CurrentAnimationTime < 0.0f)
        {
            CurrentAnimationTime += PlayLength;
        }
    }
    else
    {
        CurrentAnimationTime = FMath::Clamp(CurrentAnimationTime, 0.0f, PlayLength);
    }
}

void UAnimationSequence::Evaluate(FPoseContext& Output)
{
    if (!DataModel)
    {
        return;
    }

    if (!Output.Skeleton)
    {
        if (Skeleton.Bones.Num() == 0)
        {
            return;
        }
        Output.Skeleton = &Skeleton;
    }

    EvaluatePose(CurrentAnimationTime, Output);
}
FTransform UAnimationSequence::GetBonePose(const FName& BoneName, float Time) const
{
    if (!DataModel)
        return FTransform();

    // Bind Pose 가져오기 (키가 없을 때 사용)
    FTransform BindPoseTransform;
    if (Skeleton.Bones.Num() > 0)
    {
        // BoneName으로 본 인덱스 찾기
        auto It = Skeleton.BoneNameToIndex.find(BoneName.ToString());
        if (It != Skeleton.BoneNameToIndex.end())
        {
            int32 BoneIndex = It->second;
            if (BoneIndex >= 0 && BoneIndex < Skeleton.Bones.Num())
            {
                const FBone& Bone = Skeleton.Bones[BoneIndex];
                BindPoseTransform = FTransform(Bone.BindPose);
            }
        }
    }

    const FBoneAnimationTrack* AnimTrack = DataModel->FindTrackByBone(BoneName);
    if (!AnimTrack)
    {
        // 애니메이션 트랙이 없으면 Bind Pose 반환
        return BindPoseTransform;
    }

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

    // Position 보간 (Linear) - 키가 없으면 Bind Pose 사용
    if (Track.PosKeys.Num() > 0)
    {
        int32 PosIdx0 = FMath::Clamp(FrameIndex0, 0, Track.PosKeys.Num() - 1);
        int32 PosIdx1 = FMath::Clamp(FrameIndex1, 0, Track.PosKeys.Num() - 1);

        const FVector& Pos0 = Track.PosKeys[PosIdx0];
        const FVector& Pos1 = Track.PosKeys[PosIdx1];

        Result.Translation = FVector::Lerp(Pos0, Pos1, Alpha);
    }
    else
    {
        Result.Translation = BindPoseTransform.Translation;
    }

    // Rotation 보간 (Slerp) - 키가 없으면 Bind Pose 사용
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
    else
    {
        Result.Rotation = BindPoseTransform.Rotation;
    }

    // Scale 보간 (Linear) - 키가 없으면 Bind Pose 사용
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
        Result.Scale3D = BindPoseTransform.Scale3D;
    }

    return Result;
}

const TArray<FBoneAnimationTrack>& UAnimationSequence::GetBoneAnimationTracks() const
{
    static TArray<FBoneAnimationTrack> Empty;
    return DataModel ? DataModel->GetBoneAnimationTracks() : Empty;
}

void UAnimationSequence::EvaluatePose(float Time, FPoseContext& OutContext) const
{
    if (!DataModel)
    {
        OutContext.EvaluatedPoses.SetNum(0);
        return;
    }

    if (!OutContext.Skeleton)
    {
        if (Skeleton.Bones.Num() == 0)
        {
            OutContext.EvaluatedPoses.SetNum(0);
            return;
        }
        OutContext.Skeleton = &Skeleton;
    }

    const FSkeleton& EvalSkeleton = *OutContext.Skeleton;

    const int32 BoneNum = EvalSkeleton.Bones.Num();
    OutContext.EvaluatedPoses.SetNum(BoneNum);

    for (int32 BoneIndex = 0; BoneIndex < BoneNum; BoneIndex++)
    {
        const FBone CurBone = EvalSkeleton.Bones[BoneIndex];
        const FName BoneName(CurBone.Name);

        OutContext.EvaluatedPoses[BoneIndex] = GetBonePose(BoneName, Time);
    }
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

void UAnimationSequence::AddAnimNotify(UAnimNotify* InNotify)
{
    if (!InNotify)
        return;

    // 중복 체크
    for (UAnimNotify* Notify : AnimNotifies)
    {
        if (Notify == InNotify)
            return;
    }

    AnimNotifies.Add(InNotify);
}

void UAnimationSequence::RemoveAnimNotify(UAnimNotify* InNotify)
{
    if (!InNotify)
        return;

    AnimNotifies.Remove(InNotify);
}

void UAnimationSequence::AddAnimNotifyState(UAnimNotifyState* InNotifyState)
{
    if (!InNotifyState)
        return;

    // 중복 체크
    for (UAnimNotifyState* NotifyState : AnimNotifyStates)
    {
        if (NotifyState == InNotifyState)
            return;
    }

    AnimNotifyStates.Add(InNotifyState);
}

void UAnimationSequence::RemoveAnimNotifyState(UAnimNotifyState* InNotifyState)
{
    if (!InNotifyState)
        return;

    AnimNotifyStates.Remove(InNotifyState);
}
