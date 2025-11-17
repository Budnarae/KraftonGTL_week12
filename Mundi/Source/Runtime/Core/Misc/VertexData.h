#pragma once
#include "Archive.h"
#include "Name.h"
#include "Vector.h"
#include "Delegates.h"

// 직렬화 포맷 (FVertexDynamic와 역할이 달라서 분리됨)
struct FNormalVertex
{
    FVector pos;
    FVector normal;
    FVector2D tex;
    FVector4 Tangent;
    FVector4 color;

    friend FArchive& operator<<(FArchive& Ar, FNormalVertex& Vtx)
    {
        Ar << Vtx.pos;
        Ar << Vtx.normal;
        Ar << Vtx.Tangent;
        Ar << Vtx.color;
        Ar << Vtx.tex;
        return Ar;
    }
};

struct FMeshData
{
    TArray<FVector> Vertices;
    TArray<uint32> Indices;
    TArray<FVector4> Color;
    TArray<FVector2D> UV;
    TArray<FVector> Normal;
};

struct FVertexSimple
{
    FVector Position;
    FVector4 Color;

    void FillFrom(const FMeshData& mesh, size_t i);
    void FillFrom(const FNormalVertex& src);
};

// 런타임 포맷 (FNormalVertex와 역할이 달라서 분리됨)
struct FVertexDynamic
{
    FVector Position;
    FVector Normal;
    FVector2D UV;
    FVector4 Tangent;
    FVector4 Color;

    void FillFrom(const FMeshData& mesh, size_t i);
    void FillFrom(const FNormalVertex& src);
};

/**
* 스키닝용 정점 구조체
*/
struct FSkinnedVertex
{
    FVector Position{}; // 정점 위치
    FVector Normal{}; // 법선 벡터
    FVector2D UV{}; // 텍스처 좌표
    FVector4 Tangent{}; // 탄젠트 (w는 binormal 방향)
    FVector4 Color{}; // 정점 컬러
    uint32 BoneIndices[4]{}; // 영향을 주는 본 인덱스 (최대 4개)
    float BoneWeights[4]{}; // 각 본의 가중치 (합이 1.0)

    friend FArchive& operator<<(FArchive& Ar, FSkinnedVertex& Vertex)
    {
        Ar << Vertex.Position;
        Ar << Vertex.Normal;
        Ar << Vertex.UV;
        Ar << Vertex.Tangent;
        Ar << Vertex.Color;

        for (int i = 0; i < 4; ++i)
        {
            Ar << Vertex.BoneIndices[i];
        }

        for (int i = 0; i < 4; ++i)
        {
            Ar << Vertex.BoneWeights[i];
        }

        return Ar;
    }
};

// 같은 Position인데 Normal이나 UV가 다른 vertex가 존재할 수 있음, 그래서 SkinnedVertex를 키로 구별해야해서 hash함수 정의함
template <class T>
inline void CombineHash(size_t& InSeed, const T& Vertex)
{
    std::hash<T> Hasher;

    InSeed ^= Hasher(Vertex) + 0x9e3779b9 + (InSeed << 6) + (InSeed >> 2);
}
inline bool operator==(const FSkinnedVertex& Vertex1, const FSkinnedVertex& Vertex2)
{
    if (Vertex1.Position != Vertex2.Position ||
        Vertex1.Normal != Vertex2.Normal ||
        Vertex1.UV != Vertex2.UV ||
        Vertex1.Tangent != Vertex2.Tangent ||
        Vertex1.Color != Vertex2.Color)
    {
        return false;
    }

    if (std::memcmp(Vertex1.BoneIndices, Vertex2.BoneIndices, sizeof(Vertex1.BoneIndices)) != 0)
    {
        return false;
    }

    if (std::memcmp(Vertex1.BoneWeights, Vertex2.BoneWeights, sizeof(Vertex1.BoneWeights)) != 0)
    {
        return false;
    }

    // 모든 게 같음
    return true;
}
namespace std
{
    template<>
    struct hash<FSkinnedVertex>
    {

        size_t operator()(const FSkinnedVertex& Vertex) const
        {
            size_t Seed = 0;

            CombineHash(Seed, Vertex.Position.X); CombineHash(Seed, Vertex.Position.Y); CombineHash(Seed, Vertex.Position.Z);
            CombineHash(Seed, Vertex.Normal.X); CombineHash(Seed, Vertex.Normal.Y); CombineHash(Seed, Vertex.Normal.Z);
            CombineHash(Seed, Vertex.Tangent.X); CombineHash(Seed, Vertex.Tangent.Y); CombineHash(Seed, Vertex.Tangent.Z); CombineHash(Seed, Vertex.Tangent.W);
            CombineHash(Seed, Vertex.Color.X); CombineHash(Seed, Vertex.Color.Y); CombineHash(Seed, Vertex.Color.Z); CombineHash(Seed, Vertex.Color.W);
            CombineHash(Seed, Vertex.UV.X); CombineHash(Seed, Vertex.UV.Y);

            for (int Index = 0; Index < 4; Index++)
            {
                CombineHash(Seed, Vertex.BoneIndices[Index]);
                CombineHash(Seed, Vertex.BoneWeights[Index]);
            }
            return Seed;
        }
    };
}

struct FBillboardVertexInfo {
    FVector WorldPosition;
    FVector2D CharSize;//char scale
    FVector4 UVRect;//uv start && uv size
};

struct FBillboardVertexInfo_GPU
{
    float Position[3];
    float CharSize[2];
    float UVRect[4];

    void FillFrom(const FMeshData& mesh, size_t i); 
    void FillFrom(const FNormalVertex& src);
};

// 빌보드 전용: 위치 + UV만 있으면 충분
struct FBillboardVertex
{
    FVector WorldPosition;  // 정점 위치 (로컬 좌표, -0.5~0.5 기준 쿼드)
    FVector2D UV;        // 텍스처 좌표 (0~1)

    void FillFrom(const FMeshData& mesh, size_t i);
    void FillFrom(const FNormalVertex& src);
};

struct FGroupInfo
{
    uint32 StartIndex = 0;
    uint32 IndexCount = 0;
    FString InitialMaterialName; // obj 파일 자체에 맵핑된 material 이름

    friend FArchive& operator<<(FArchive& Ar, FGroupInfo& Info)
    {
        Ar << Info.StartIndex;
        Ar << Info.IndexCount;

        if (Ar.IsSaving())
            Serialization::WriteString(Ar, Info.InitialMaterialName);
        else if (Ar.IsLoading())
            Serialization::ReadString(Ar, Info.InitialMaterialName);

        return Ar;
    }
};

struct FStaticMesh
{
    FString PathFileName;
    FString CacheFilePath;  // 캐시된 소스 경로 (예: DerivedDataCache/cube.obj.bin)

    TArray<uint32> Indices;
    TArray<FNormalVertex> Vertices;
    TArray<FGroupInfo> GroupInfos; // 각 group을 render 하기 위한 정보

    bool bHasMaterial;

    friend FArchive& operator<<(FArchive& Ar, FStaticMesh& Mesh)
    {
        if (Ar.IsSaving())
        {
            Serialization::WriteString(Ar, Mesh.PathFileName);
            Serialization::WriteArray(Ar, Mesh.Vertices);
            Serialization::WriteArray(Ar, Mesh.Indices);

            uint32_t gCount = (uint32_t)Mesh.GroupInfos.size();
            Ar << gCount;
            for (auto& g : Mesh.GroupInfos) Ar << g;

            Ar << Mesh.bHasMaterial;
        }
        else if (Ar.IsLoading())
        {
            Serialization::ReadString(Ar, Mesh.PathFileName);
            Serialization::ReadArray(Ar, Mesh.Vertices);
            Serialization::ReadArray(Ar, Mesh.Indices);

            uint32_t gCount;
            Ar << gCount;
            Mesh.GroupInfos.resize(gCount);
            for (auto& g : Mesh.GroupInfos) Ar << g;

            Ar << Mesh.bHasMaterial;
        }
        return Ar;
    }
};

struct FBone
{
    FString Name; // 본 이름
    int32 ParentIndex; // 부모 본 인덱스 (-1이면 루트)
    FMatrix BindPose; // Bind Pose 변환 행렬
    FMatrix InverseBindPose; // Inverse Bind Pose (스키닝용)

    friend FArchive& operator<<(FArchive& Ar, FBone& Bone)
    {
        if (Ar.IsSaving())
        {
            Serialization::WriteString(Ar, Bone.Name);
            Ar << Bone.ParentIndex;
            Ar << Bone.BindPose;
            Ar << Bone.InverseBindPose;
        }
        else if (Ar.IsLoading())
        {
            Serialization::ReadString(Ar, Bone.Name);
            Ar << Bone.ParentIndex;
            Ar << Bone.BindPose;
            Ar << Bone.InverseBindPose;
        }
        return Ar;
    }
};

struct FSkeleton
{
    FString Name; // 스켈레톤 이름
    TArray<FBone> Bones; // 본 배열
    TMap <FString, int32> BoneNameToIndex; // 이름으로 본 검색

    friend FArchive& operator<<(FArchive& Ar, FSkeleton& Skeleton)
    {
        if (Ar.IsSaving())
        {
            Serialization::WriteString(Ar, Skeleton.Name);

            uint32 boneCount = static_cast<uint32>(Skeleton.Bones.size());
            Ar << boneCount;
            for (auto& bone : Skeleton.Bones)
            {
                Ar << bone;
            }
            // BoneNameToIndex는 로드 시 재구축 가능하므로 저장 안 함
        }
        else if (Ar.IsLoading())
        {
            Serialization::ReadString(Ar, Skeleton.Name);

            uint32 boneCount;
            Ar << boneCount;
            Skeleton.Bones.resize(boneCount);
            for (auto& bone : Skeleton.Bones)
            {
                Ar << bone;
            }

            // BoneNameToIndex 재구축
            Skeleton.BoneNameToIndex.clear();
            for (int32 i = 0; i < static_cast<int32>(Skeleton.Bones.size()); ++i)
            {
                Skeleton.BoneNameToIndex[Skeleton.Bones[i].Name] = i;
            }
        }
        return Ar;
    }
};

struct FVertexWeight
{
    uint32 VertexIndex; // 정점 인덱스
    float Weight; // 가중치
};

struct FSkeletalMeshData
{
    FString PathFileName;
    FString CacheFilePath;
    
    TArray<FSkinnedVertex> Vertices; // 정점 배열
    TArray<uint32> Indices; // 인덱스 배열
    FSkeleton Skeleton; // 스켈레톤 정보
    TArray<FGroupInfo> GroupInfos; // 머티리얼 그룹 (기존 시스템 재사용)
    bool bHasMaterial = false;

    friend FArchive& operator<<(FArchive& Ar, FSkeletalMeshData& Data)
    {
        if (Ar.IsSaving())
        {
            // 1. Vertices 저장
            Serialization::WriteArray(Ar, Data.Vertices);

            // 2. Indices 저장
            Serialization::WriteArray(Ar, Data.Indices);

            // 3. Skeleton 저장
            Ar << Data.Skeleton;

            // 4. GroupInfos 저장
            uint32 gCount = static_cast<uint32>(Data.GroupInfos.size());
            Ar << gCount;
            for (auto& g : Data.GroupInfos)
            {
                Ar << g;
            }

            // 5. Material 플래그 저장
            Ar << Data.bHasMaterial;

            // 6. CacheFilePath 저장
            Serialization::WriteString(Ar, Data.CacheFilePath);
        }
        else if (Ar.IsLoading())
        {
            // 1. Vertices 로드
            Serialization::ReadArray(Ar, Data.Vertices);

            // 2. Indices 로드
            Serialization::ReadArray(Ar, Data.Indices);

            // 3. Skeleton 로드
            Ar << Data.Skeleton;

            // 4. GroupInfos 로드
            uint32 gCount;
            Ar << gCount;
            Data.GroupInfos.resize(gCount);
            for (auto& g : Data.GroupInfos)
            {
                Ar << g;
            }

            // 5. Material 플래그 로드
            Ar << Data.bHasMaterial;

            // 6. CacheFilePath 로드
            Serialization::ReadString(Ar, Data.CacheFilePath);
        }
        return Ar;
    }
};

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
    TArray<FTransform> EvaluatedPoses;
};

// 전방 선언
class UAnimNodeTransitionRule;

// Forward declaration
struct FAnimState;

struct FAnimStateTransition
{
    // ------------------------------------------------------------------------
    // Source / Target State
    // Transition 출발 상태와 도착 상태 포인터
    // ------------------------------------------------------------------------
    FAnimState* SourceState = nullptr;
    FAnimState* TargetState = nullptr;

    uint32 Index{};   // Animation State Machine에서의 Index

    // ------------------------------------------------------------------------
    // Transition 조건
    // true가 되면 Transition 발동
    // ------------------------------------------------------------------------
    bool CanEnterTransition = false;

    // Delegate 관리
    UAnimNodeTransitionRule* AssociatedRule = nullptr;
    FDelegateHandle DelegateHandle;

    // 생성자/소멸자
    FAnimStateTransition() = default;
    FAnimStateTransition(const FAnimStateTransition& Other);
    FAnimStateTransition& operator=(const FAnimStateTransition& Other);
    ~FAnimStateTransition();

    // Delegate Handle 정리 (구현은 VertexData.cpp에)
    void CleanupDelegate();
    
    /* 이하는 나중에 해제하여 사용할 것 */
    
    // ------------------------------------------------------------------------
    // Transition 블렌딩 시간
    // ActiveState Pose -> TargetState Pose로 자연스럽게 Blend
    // ------------------------------------------------------------------------
    float BlendTime = 0.2f;

    // ------------------------------------------------------------------------
    // Transition 블렌딩 경과 시간
    // 경과 시간이 지나면 다음 State로 전환
    // ------------------------------------------------------------------------
    float BlendTimeElapsed = 0.f;

    // ------------------------------------------------------------------------
    // Transition 진행 상태
    // BlendAlpha: 0.0 = SourcePose, 1.0 = TargetPose
    // ------------------------------------------------------------------------
    float BlendAlpha = 0.0f;

    bool bIsBlending = false;

    // ------------------------------------------------------------------------
    // Interrupt 옵션
    // 다른 Transition으로 중단 가능한지 여부
    // ------------------------------------------------------------------------
    //bool bCanInterrupt = true;

    // ------------------------------------------------------------------------
    // Blending 시작 시 파라미터 세팅
    // ------------------------------------------------------------------------
    void StartBlending()
    {
        bIsBlending = true;
        BlendTimeElapsed = 0.f;
        BlendAlpha = 0.f;
    }
    
    // TODO: Blending Helper 함수 구현 이후 Update Evaluate 구현
    // ------------------------------------------------------------------------
    // Update
    // DeltaTime 기반으로 BlendAlpha 계산
    // Condition 평가 후 Blend 진행
    // ------------------------------------------------------------------------
    void Update(const FAnimationUpdateContext& Context)
    {
        BlendTimeElapsed += Context.DeltaTime;
        if (BlendTimeElapsed >= BlendTime)
        {
            bIsBlending = false;
            return;
        }

        // 블렌딩은 추후 구현
    }

    // ------------------------------------------------------------------------
    // Evaluate
    // SourcePose와 TargetPose를 BlendAlpha 기준으로 보간
    // ------------------------------------------------------------------------
    void Evaluate(FPoseContext& Output)
    {
        // 블렌딩은 추후 구현
    }

    // ------------------------------------------------------------------------
    // Transition Condition Converter
    // 특정 조건을 충족하면 외부의 delegate에서 호출
    // ------------------------------------------------------------------------
    void TriggerTransition()
    {
        CanEnterTransition = true;
    }

    // ------------------------------------------------------------------------
    // Setter for BlendTime
    // ------------------------------------------------------------------------
    void SetBlendTime(float InBlendTime)
    {
        BlendTime = InBlendTime;
    }
};

class UAnimationSequence;

struct FAnimState
{
    FAnimState() = default;
    ~FAnimState() = default;
    
    FName Name{};
    uint32 Index{};   // Animation State Machine에서의 Index
    TArray<UAnimationSequence*> AnimSequences;

    /**
     * @brief AnimSequence를 이 State에 추가
     */
    void AddAnimSequence(UAnimationSequence* AnimSequence)
    {
        if (AnimSequence)
        {
            AnimSequences.Add(AnimSequence);
        }
    }
};