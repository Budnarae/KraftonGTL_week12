# 스켈레탈 메시 시스템 분석

## 1. 핵심 데이터 구조

### 1.1 FBone 구조체
**파일**: `Mundi\Source\Runtime\Core\Misc\VertexData.h:237-262`

```cpp
struct FBone
{
    FString Name;                // 본 이름
    int32 ParentIndex;           // 부모 본 인덱스 (-1이면 루트)
    FMatrix BindPose;            // Bind Pose 변환 행렬
    FMatrix InverseBindPose;     // Inverse Bind Pose (스키닝용)
};
```

### 1.2 FSkeleton 구조체
**파일**: `Mundi\Source\Runtime\Core\Misc\VertexData.h:264-305`

```cpp
struct FSkeleton
{
    FString Name;                           // 스켈레톤 이름
    TArray<FBone> Bones;                    // 본 배열
    TMap<FString, int32> BoneNameToIndex;   // 이름으로 본 검색
};
```

### 1.3 FSkinnedVertex 구조체
**파일**: `Mundi\Source\Runtime\Core\Misc\VertexData.h:59-89`

```cpp
struct FSkinnedVertex
{
    FVector Position;           // 정점 위치
    FVector Normal;             // 법선 벡터
    FVector2D UV;               // 텍스처 좌표
    FVector4 Tangent;           // 탄젠트 (w는 binormal 방향)
    FVector4 Color;             // 정점 컬러
    uint32 BoneIndices[4];      // 영향을 주는 본 인덱스 (최대 4개)
    float BoneWeights[4];       // 각 본의 가중치 (합이 1.0)
};
```

### 1.4 FSkeletalMeshData 구조체
**파일**: `Mundi\Source\Runtime\Core\Misc\VertexData.h:313-379`

```cpp
struct FSkeletalMeshData
{
    FString PathFileName;
    FString CacheFilePath;

    TArray<FSkinnedVertex> Vertices;   // 정점 배열
    TArray<uint32> Indices;            // 인덱스 배열
    FSkeleton Skeleton;                // 스켈레톤 정보
    TArray<FGroupInfo> GroupInfos;     // 머티리얼 그룹
    bool bHasMaterial;
};
```

## 2. 클래스 계층 구조

```
UResourceBase
    └── USkeletalMesh (에셋)

UMeshComponent
    └── USkinnedMeshComponent (CPU 스키닝)
            └── USkeletalMeshComponent (애니메이션 포즈)

AActor
    └── ASkeletalMeshActor
```

## 3. USkeletalMesh 클래스

**파일**: `Mundi\Source\Runtime\AssetManagement\SkeletalMesh.h`

스켈레탈 메시 에셋을 관리하는 클래스입니다.

**주요 기능**:
- FBX 파일에서 로드된 데이터 관리
- GPU 버퍼 관리 (IndexBuffer)
- CPU 스키닝 방식이므로 VertexBuffer는 컴포넌트가 소유

**핵심 메서드**:
```cpp
void Load(const FString& InFilePath, ID3D11Device* InDevice);
void CreateVertexBuffer(ID3D11Buffer** InVertexBuffer);
void UpdateVertexBuffer(const TArray<FNormalVertex>& SkinnedVertices, ID3D11Buffer* InVertexBuffer);
```

## 4. USkinnedMeshComponent 클래스

**파일**: `Mundi\Source\Runtime\Engine\Components\SkinnedMeshComponent.cpp`

CPU 스키닝을 수행하는 컴포넌트입니다.

### 4.1 CPU 스키닝 수행
**라인**: 233-323

```cpp
void USkinnedMeshComponent::PerformSkinning()
{
    const TArray<FSkinnedVertex>& SrcVertices = SkeletalMesh->GetSkeletalMeshData()->Vertices;
    const int32 NumVertices = SrcVertices.Num();
    SkinnedVertices.SetNum(NumVertices);

    for (int32 Idx = 0; Idx < NumVertices; ++Idx)
    {
        const FSkinnedVertex& SrcVert = SrcVertices[Idx];
        FNormalVertex& DstVert = SkinnedVertices[Idx];

        DstVert.pos = SkinVertexPosition(SrcVert);      // 위치 스키닝
        DstVert.normal = SkinVertexNormal(SrcVert);     // 노말 스키닝
        DstVert.Tangent = SkinVertexTangent(SrcVert);   // 탄젠트 스키닝
        DstVert.tex = SrcVert.UV;
    }
}
```

### 4.2 정점 위치 스키닝 (Linear Blend Skinning)
**라인**: 261-279

```cpp
FVector USkinnedMeshComponent::SkinVertexPosition(const FSkinnedVertex& InVertex) const
{
    FVector BlendedPosition(0.f, 0.f, 0.f);

    for (int32 Idx = 0; Idx < 4; ++Idx)
    {
        const uint32 BoneIndex = InVertex.BoneIndices[Idx];
        const float Weight = InVertex.BoneWeights[Idx];

        if (Weight > 0.f)
        {
            const FMatrix& SkinMatrix = FinalSkinningMatrices[BoneIndex];
            FVector TransformedPosition = SkinMatrix.TransformPosition(InVertex.Position);
            BlendedPosition += TransformedPosition * Weight;
        }
    }

    return BlendedPosition;
}
```

### 4.3 노말 스키닝 (역전치 행렬 사용)
**라인**: 281-299

```cpp
FVector USkinnedMeshComponent::SkinVertexNormal(const FSkinnedVertex& InVertex) const
{
    FVector BlendedNormal(0.f, 0.f, 0.f);

    for (int32 Idx = 0; Idx < 4; ++Idx)
    {
        const uint32 BoneIndex = InVertex.BoneIndices[Idx];
        const float Weight = InVertex.BoneWeights[Idx];

        if (Weight > 0.f)
        {
            // 노말은 역전치 행렬로 변환
            const FMatrix& SkinMatrix = FinalSkinningNormalMatrices[BoneIndex];
            FVector TransformedNormal = SkinMatrix.TransformVector(InVertex.Normal);
            BlendedNormal += TransformedNormal * Weight;
        }
    }

    return BlendedNormal.GetSafeNormal();
}
```

### 4.4 탄젠트 스키닝
**라인**: 301-323

```cpp
FVector4 USkinnedMeshComponent::SkinVertexTangent(const FSkinnedVertex& InVertex) const
{
    FVector TangentDir(InVertex.Tangent.X, InVertex.Tangent.Y, InVertex.Tangent.Z);
    float Handedness = InVertex.Tangent.W;

    FVector BlendedTangent(0.f, 0.f, 0.f);

    for (int32 Idx = 0; Idx < 4; ++Idx)
    {
        const uint32 BoneIndex = InVertex.BoneIndices[Idx];
        const float Weight = InVertex.BoneWeights[Idx];

        if (Weight > 0.f)
        {
            const FMatrix& SkinMatrix = FinalSkinningMatrices[BoneIndex];
            FVector TransformedTangent = SkinMatrix.TransformVector(TangentDir);
            BlendedTangent += TransformedTangent * Weight;
        }
    }

    FVector FinalTangent = BlendedTangent.GetSafeNormal();
    return FVector4(FinalTangent.X, FinalTangent.Y, FinalTangent.Z, Handedness);
}
```

## 5. USkeletalMeshComponent 클래스

**파일**: `Mundi\Source\Runtime\Engine\Components\SkeletalMeshComponent.cpp`

애니메이션 포즈를 관리하는 컴포넌트입니다.

### 5.1 핵심 데이터

```cpp
// 각 본의 부모 기준 로컬 트랜스폼
TArray<FTransform> CurrentLocalSpacePose;

// LocalSpacePose로부터 계산된 컴포넌트 기준 트랜스폼
TArray<FTransform> CurrentComponentSpacePose;

// 부모에게 보낼 최종 스키닝 행렬
TArray<FMatrix> TempFinalSkinningMatrices;

// CPU 스키닝에 전달할 최종 노말 스키닝 행렬
TArray<FMatrix> TempFinalSkinningNormalMatrices;
```

### 5.2 포즈 계산 파이프라인

```cpp
void USkeletalMeshComponent::ForceRecomputePose()
{
    // 1단계: LocalSpace -> ComponentSpace 계산
    UpdateComponentSpaceTransforms();

    // 2단계: ComponentSpace -> Final Skinning Matrices 계산
    UpdateFinalSkinningMatrices();

    // 3단계: 부모 클래스에 스키닝 행렬 전달
    UpdateSkinningMatrices(TempFinalSkinningMatrices, TempFinalSkinningNormalMatrices);

    // 4단계: CPU 스키닝 수행
    PerformSkinning();
}
```

### 5.3 컴포넌트 스페이스 변환
**라인**: 143-163

```cpp
void USkeletalMeshComponent::UpdateComponentSpaceTransforms()
{
    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    const int32 NumBones = Skeleton.Bones.Num();

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FTransform& LocalTransform = CurrentLocalSpacePose[BoneIndex];
        const int32 ParentIndex = Skeleton.Bones[BoneIndex].ParentIndex;

        if (ParentIndex == -1) // 루트 본
        {
            CurrentComponentSpacePose[BoneIndex] = LocalTransform;
        }
        else // 자식 본
        {
            const FTransform& ParentComponentTransform = CurrentComponentSpacePose[ParentIndex];
            // 부모의 월드 트랜스폼과 자식의 로컬 트랜스폼 합성
            CurrentComponentSpacePose[BoneIndex] = ParentComponentTransform.GetWorldTransform(LocalTransform);
        }
    }
}
```

### 5.4 최종 스키닝 행렬 계산
**라인**: 165-178

```cpp
void USkeletalMeshComponent::UpdateFinalSkinningMatrices()
{
    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    const int32 NumBones = Skeleton.Bones.Num();

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FMatrix& InvBindPose = Skeleton.Bones[BoneIndex].InverseBindPose;
        const FMatrix ComponentPoseMatrix = CurrentComponentSpacePose[BoneIndex].ToMatrix();

        // 스키닝 행렬 = InverseBindPose * CurrentPose
        TempFinalSkinningMatrices[BoneIndex] = InvBindPose * ComponentPoseMatrix;

        // 노말용 역전치 행렬
        TempFinalSkinningNormalMatrices[BoneIndex] = TempFinalSkinningMatrices[BoneIndex].Inverse().Transpose();
    }
}
```

## 6. FTransform 구조체

**파일**: `Mundi\Source\Runtime\Core\Math\Vector.h:1220-1260`

```cpp
struct FTransform
{
    FVector Translation;
    FQuat   Rotation;
    FVector Scale3D;

    // 자식 로컬 좌표계를 부모 좌표계로 변환
    FTransform GetWorldTransform(const FTransform& ChildTransform) const
    {
        FTransform Result;

        // 회전 결합 (자식 먼저)
        Result.Rotation = Rotation * ChildTransform.Rotation;
        Result.Rotation.Normalize();

        // 스케일 결합 (component-wise)
        Result.Scale3D = FVector(
            Scale3D.X * ChildTransform.Scale3D.X,
            Scale3D.Y * ChildTransform.Scale3D.Y,
            Scale3D.Z * ChildTransform.Scale3D.Z
        );

        // 이동 결합: Scale -> Rotate -> Translate
        FVector Scaled(ChildTransform.Translation.X * Scale3D.X,
                      ChildTransform.Translation.Y * Scale3D.Y,
                      ChildTransform.Translation.Z * Scale3D.Z);
        FVector Rotated = Rotation.RotateVector(Scaled);
        Result.Translation = Translation + Rotated;

        return Result;
    }

    FMatrix ToMatrix() const;  // 행렬로 변환
};
```

## 7. 렌더링 파이프라인

**파일**: `Mundi\Source\Runtime\Engine\Components\SkinnedMeshComponent.cpp:47-145`

```cpp
void USkinnedMeshComponent::CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View)
{
    // 스키닝 행렬이 변경되었으면 버텍스 버퍼 업데이트
    if (bSkinningMatricesDirty)
    {
        bSkinningMatricesDirty = false;
        SkeletalMesh->UpdateVertexBuffer(SkinnedVertices, VertexBuffer);
    }

    // 머티리얼 그룹별로 배치 생성
    // ...
}
```

## 8. 주요 특징

1. **CPU 스키닝 방식**: GPU가 아닌 CPU에서 스키닝 계산 수행
2. **Linear Blend Skinning**: 최대 4개 본의 가중치 블렌딩
3. **역전치 행렬**: 노말 벡터는 역전치 행렬로 변환하여 비균등 스케일 대응
4. **계층적 본 구조**: 부모-자식 관계로 본 계층 표현
5. **로컬/컴포넌트 스페이스 분리**: 애니메이션 편집과 렌더링 분리
