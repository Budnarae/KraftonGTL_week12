#pragma once

struct FRawAnimSequenceTrack
{
    TArray<FVector> PosKeys; // 위치 키프레임
    TArray<FVector4>   RotKeys; // 회전 키프레임 (Quaternion)
    TArray<FVector> ScaleKeys; // 스케일 키프레임

    friend FArchive& operator<<(FArchive& Ar, FRawAnimSequenceTrack& Track)
    {
        if (Ar.IsSaving())
        {
            Serialization::WriteArray(Ar, Track.PosKeys);
            Serialization::WriteArray(Ar, Track.RotKeys);
            Serialization::WriteArray(Ar, Track.ScaleKeys);
        }
        else if (Ar.IsLoading())
        {
            Serialization::ReadArray(Ar, Track.PosKeys);
            Serialization::ReadArray(Ar, Track.RotKeys);
            Serialization::ReadArray(Ar, Track.ScaleKeys);
        }
        return Ar;
    }
};

struct FBoneAnimationTrack
{
    FName Name;                        // Bone 이름
    FRawAnimSequenceTrack InternalTrack; // 실제 애니메이션 데이터

    friend FArchive& operator<<(FArchive& Ar, FBoneAnimationTrack& Track)
    {
        if (Ar.IsSaving())
        {
            FString NameStr = Track.Name.ToString();
            Serialization::WriteString(Ar, NameStr);
            Ar << Track.InternalTrack;
        }
        else if (Ar.IsLoading())
        {
            FString NameStr;
            Serialization::ReadString(Ar, NameStr);
            Track.Name = FName(NameStr);
            Ar << Track.InternalTrack;
        }
        return Ar;
    }
};

namespace Serialization {
    template<>
    inline void WriteArray<FBoneAnimationTrack>(FArchive& Ar, const TArray<FBoneAnimationTrack>& Arr) {
        uint32 Count = (uint32)Arr.size();
        Ar << Count;
        for (auto& Track : Arr) Ar << const_cast<FBoneAnimationTrack&>(Track);
    }

    template<>
    inline void ReadArray<FBoneAnimationTrack>(FArchive& Ar, TArray<FBoneAnimationTrack>& Arr) {
        uint32 Count;
        Ar << Count;
        Arr.resize(Count);
        for (auto& Track : Arr) Ar << Track;
    }
}

struct FAnimationUpdateContext
{
    // ------------------------------------------------------------------------
    // DeltaTime
    // 현재 Tick 동안 경과한 시간 (초)
    // 모든 AnimNode Update에서 참조
    // ------------------------------------------------------------------------
    float DeltaTime{};

    // ------------------------------------------------------------------------
    // bIsSkeletonInitialised
    // SkeletalMesh가 초기화되었는지 여부
    // Update 수행 전 체크
    // ------------------------------------------------------------------------
    // bool bIsSkeletonInitialized;

    // ------------------------------------------------------------------------
    // bEnableRootMotion
    // RootMotion 적용 여부
    // ------------------------------------------------------------------------
    // bool bEnableRootMotion;

    /* 추후 필요한 정보를 추가 */
};

struct FPoseContext
{
    const FSkeleton* Skeleton = nullptr;
    TArray<FTransform> EvaluatedPoses;

    FPoseContext() = default;

    explicit FPoseContext(const FSkeleton* InSkeleton)
        : Skeleton(InSkeleton)
    {
        if (Skeleton)
        {
            EvaluatedPoses.SetNum(Skeleton->Bones.Num());
        }
    }

    FPoseContext(const FPoseContext& Other)
        : Skeleton(Other.Skeleton)
    {
        if (Skeleton)
        {
            EvaluatedPoses.SetNum(Skeleton->Bones.Num());
        }
    }

    FPoseContext& operator=(const FPoseContext& Other)
    {
        if (this != &Other)
        {
            Skeleton = Other.Skeleton;
            if (Skeleton)
            {
                EvaluatedPoses.SetNum(Skeleton->Bones.Num());
            }
            else
            {
                EvaluatedPoses.Empty();
            }
        }
        return *this;
    }

    void ResetToRefPose()
    {
        if (!Skeleton)
        {
            EvaluatedPoses.Empty();
            return;
        }

        const int32 NumBones = static_cast<int32>(Skeleton->Bones.Num());
        EvaluatedPoses.SetNum(NumBones);

        // 1차 버전: 일단 Identity Pose로 채운다.
        // (나중에 BindPose 기반 로컬 RefPose를 Skeleton에 캐시해서 써도 됨)
        for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
        {
            EvaluatedPoses[BoneIndex] = FTransform();
        }
    }

    void CopyFrom(const FPoseContext& Other)
    {
        Skeleton = Other.Skeleton;
        EvaluatedPoses = Other.EvaluatedPoses;
    }
};