# FBX Importer 분석

## 1. 개요

Autodesk FBX SDK를 사용하여 FBX 파일로부터 스켈레탈 메시, 스켈레톤, 스킨 가중치 데이터를 임포트하는 시스템입니다.

## 2. 주요 파일 구조

### 2.1 FBX 임포터 코어
- **Mundi\Source\Editor\FBXLoader.h**
- **Mundi\Source\Editor\FBXLoader.cpp**

### 2.2 데이터 구조체
- **Mundi\Source\Runtime\Core\Misc\VertexData.h**
  - FSkinnedVertex, FBone, FSkeleton, FSkeletalMeshData

- **Mundi\Source\Runtime\Core\Misc\ResourceData.h**
  - FMaterialInfo (Phong/Lambert 모델)

## 3. UFbxLoader 클래스

싱글톤 패턴의 FBX 로더 클래스입니다.

```cpp
class UFbxLoader : public UObject
{
    FbxManager* SdkManager;
    TArray<FMaterialInfo> MaterialInfos;
};
```

### 3.1 LoadFbxMeshAsset
**파일**: `Mundi\Source\Editor\FBXLoader.cpp:105-352`

FBX 파일을 파싱하여 FSkeletalMeshData를 생성하는 메인 함수입니다.

```cpp
FSkeletalMeshData* LoadFbxMeshAsset(const FString& FilePath)
```

#### 주요 처리 단계

**1단계: 캐시 검증 및 로드 (라인 110-211)**
```cpp
// .bin 캐시 파일 존재 여부 확인
// 타임스탬프 비교 (FBX vs BIN)
// 캐시 로드 성공 시 바로 반환
```

**2단계: FBX SDK 초기화 (라인 220-237)**
```cpp
FbxImporter* Importer = FbxImporter::Create(SdkManager, "");
Importer->Initialize(FilePath, -1, SdkManager->GetIOSettings());
FbxScene* Scene = FbxScene::Create(SdkManager, "My Scene");
Importer->Import(Scene);
```

**3단계: 좌표계 변환 (라인 239-248)**
```cpp
FbxAxisSystem UnrealImportAxis(
    FbxAxisSystem::eZAxis,      // Z-Up
    FbxAxisSystem::eParityEven,
    FbxAxisSystem::eLeftHanded  // 왼손 좌표계
);
UnrealImportAxis.DeepConvertScene(Scene);
FbxSystemUnit::m.ConvertScene(Scene); // 미터 단위로 변환
```

**4단계: 삼각화 (라인 251-259)**
```cpp
FbxGeometryConverter GeometryConverter(SdkManager);
GeometryConverter.Triangulate(Scene, true); // 모든 폴리곤을 삼각형으로
```

**5단계: 2-Pass 로딩 (라인 289-296)**
- **1st Pass**: 스켈레톤 구조 로드 및 본 인덱스 할당
- **2nd Pass**: 메시 데이터 로드 및 스킨 가중치 매핑

**6단계: 캐시 저장 (라인 324-349)**
```cpp
// 메시 데이터를 .bin 파일로 직렬화
// 머티리얼을 별도 .mat.bin 파일로 저장
```

### 3.2 LoadSkeletonFromNode
**파일**: `Mundi\Source\Editor\FBXLoader.cpp:456-496`

깊이 우선 탐색(DFS)으로 본 계층 구조를 순회하며 스켈레톤을 구축합니다.

```cpp
void LoadSkeletonFromNode(
    FbxNode* InNode,
    FSkeletalMeshData& MeshData,
    int32 ParentNodeIndex,
    TMap<FbxNode*, int32>& BoneToIndex
)
{
    int32 BoneIndex = ParentNodeIndex;
    for (int Index = 0; Index < InNode->GetNodeAttributeCount(); Index++)
    {
        FbxNodeAttribute* Attribute = InNode->GetNodeAttributeByIndex(Index);

        if (Attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
        {
            FBone BoneInfo{};
            BoneInfo.Name = FString(InNode->GetName());
            BoneInfo.ParentIndex = ParentNodeIndex;

            MeshData.Skeleton.Bones.Add(BoneInfo);
            BoneIndex = MeshData.Skeleton.Bones.Num() - 1;

            MeshData.Skeleton.BoneNameToIndex.Add(BoneInfo.Name, BoneIndex);
            BoneToIndex.Add(InNode, BoneIndex);
            break;
        }
    }

    // 깊이 우선 탐색으로 자식 본 처리
    for (int Index = 0; Index < InNode->GetChildCount(); Index++)
    {
        LoadSkeletonFromNode(InNode->GetChild(Index), MeshData, BoneIndex, BoneToIndex);
    }
}
```

### 3.3 LoadMesh
**파일**: `Mundi\Source\Editor\FBXLoader.cpp:514-987`

정점, 인덱스, 스킨 가중치를 추출하는 핵심 메서드입니다.

#### 1단계: 스킨 클러스터 처리 (라인 533-579)

```cpp
// 스킨 Deformer 확인
if (InMesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
{
    for (int Index = 0; Index < ((FbxSkin*)InMesh->GetDeformer(0, FbxDeformer::eSkin))->GetClusterCount(); Index++)
    {
        FbxCluster* Cluster = ((FbxSkin*)InMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(Index);

        // BindPose, InverseBindPose 저장
        FbxAMatrix BoneBindGlobal;
        Cluster->GetTransformLinkMatrix(BoneBindGlobal);
        FbxAMatrix BoneBindGlobalInv = BoneBindGlobal.Inverse();

        // FbxMatrix(128바이트) -> FMatrix(64바이트) 원소 단위 복사
        for (int Row = 0; Row < 4; Row++)
        {
            for (int Col = 0; Col < 4; Col++)
            {
                MeshData.Skeleton.Bones[BoneToIndex[Cluster->GetLink()]].BindPose.M[Row][Col] =
                    static_cast<float>(BoneBindGlobal[Row][Col]);
                MeshData.Skeleton.Bones[BoneToIndex[Cluster->GetLink()]].InverseBindPose.M[Row][Col] =
                    static_cast<float>(BoneBindGlobalInv[Row][Col]);
            }
        }

        // 본이 영향을 주는 정점 및 가중치 저장
        int* Indices = Cluster->GetControlPointIndices();
        double* Weights = Cluster->GetControlPointWeights();

        for (int i = 0; i < Cluster->GetControlPointIndicesCount(); i++)
        {
            int CPIndex = Indices[i];
            ControlPointToBoneWeight[CPIndex].Add({BoneToIndex[Cluster->GetLink()], Weights[i]});
        }
    }
}
```

#### 2단계: 폴리곤 순회 및 정점 데이터 추출 (라인 607-885)

```cpp
for (int PolyIdx = 0; PolyIdx < PolygonCount; PolyIdx++)
{
    for (int VertIdx = 0; VertIdx < 3; VertIdx++) // 삼각형
    {
        FSkinnedVertex Vertex;
        int CPIndex = InMesh->GetPolygonVertex(PolyIdx, VertIdx);

        // 위치
        FbxVector4 Pos = FbxSceneWorld.MultT(ControlPoints[CPIndex]);
        Vertex.Position = FVector(Pos[0], Pos[1], Pos[2]);

        // 스킨 가중치
        if (ControlPointToBoneWeight.Contains(CPIndex))
        {
            const TArray<IndexWeight>& Weights = ControlPointToBoneWeight[CPIndex];
            double TotalWeight = 0.0;
            for (int i = 0; i < min(4, Weights.Num()); i++)
                TotalWeight += Weights[i].BoneWeight;

            // 최대 4개 본 가중치 (정규화)
            for (int i = 0; i < min(4, Weights.Num()); i++)
            {
                Vertex.BoneIndices[i] = Weights[i].BoneIndex;
                Vertex.BoneWeights[i] = Weights[i].BoneWeight / TotalWeight;
            }
        }

        // 법선, 탄젠트, UV 추출...
    }
}
```

#### 3단계: 탄젠트 계산 (라인 890-986)

FBX에 탄젠트가 없을 경우 Lengyel의 MikkTSpace 알고리즘으로 계산합니다.

```cpp
// 트라이앵글 기반 탄젠트/바이탄젠트 누적
for (int i = 0; i < Indices.Num(); i += 3)
{
    FVector P0 = Vertices[Indices[i + 0]].Position;
    FVector P1 = Vertices[Indices[i + 1]].Position;
    FVector P2 = Vertices[Indices[i + 2]].Position;

    FVector2D W0 = Vertices[Indices[i + 0]].UV;
    FVector2D W1 = Vertices[Indices[i + 1]].UV;
    FVector2D W2 = Vertices[Indices[i + 2]].UV;

    FVector Edge1 = P1 - P0;
    FVector Edge2 = P2 - P0;
    FVector2D DeltaUV1 = W1 - W0;
    FVector2D DeltaUV2 = W2 - W0;

    float r = 1.0f / (DeltaUV1.X * DeltaUV2.Y - DeltaUV1.Y * DeltaUV2.X);
    FVector T = (Edge1 * DeltaUV2.Y - Edge2 * DeltaUV1.Y) * r;
    FVector B = (Edge2 * DeltaUV1.X - Edge1 * DeltaUV2.X) * r;

    TempTangents[i0] += T;
    TempBitangents[i0] += B;
}

// Gram-Schmidt 직교화
for (int i = 0; i < Vertices.Num(); i++)
{
    FVector N = Vertices[i].Normal;
    FVector T = TempTangents[i];
    FVector B = TempBitangents[i];

    FVector Tangent = (T - N * FVector::DotProduct(T, N)).GetSafeNormal();
    float Handedness = FVector::DotProduct(FVector::CrossProduct(Tangent, N), B) > 0.0f ? 1.0f : -1.0f;

    Vertices[i].Tangent = FVector4(Tangent.X, Tangent.Y, Tangent.Z, Handedness);
}
```

### 3.4 ParseMaterial
**파일**: `Mundi\Source\Editor\FBXLoader.cpp:991-1076`

Phong/Lambert 머티리얼을 파싱합니다.

```cpp
FMaterialInfo UFbxLoader::ParseMaterial(FbxSurfaceMaterial* Material)
{
    FMaterialInfo MaterialInfo;

    // Phong 모델
    if (Material->GetClassId().Is(FbxSurfacePhong::ClassId))
    {
        FbxSurfacePhong* Phong = (FbxSurfacePhong*)Material;
        MaterialInfo.DiffuseColor = Phong->Diffuse.Get();
        MaterialInfo.SpecularColor = Phong->Specular.Get();
        MaterialInfo.SpecularExponent = Phong->Shininess.Get();
    }

    // 텍스처 경로 추출
    FbxProperty Property = Material->FindProperty(FbxSurfaceMaterial::sDiffuse);
    MaterialInfo.DiffuseTextureFileName = ParseTexturePath(Property);

    return MaterialInfo;
}
```

### 3.5 EnsureSingleRootBone
**파일**: `Mundi\Source\Editor\FBXLoader.cpp:1125-1180`

여러 루트 본이 있을 경우 가상 루트 본을 생성합니다.

```cpp
void UFbxLoader::EnsureSingleRootBone(FSkeletalMeshData& MeshData)
{
    TArray<int32> RootBoneIndices;
    for (int i = 0; i < MeshData.Skeleton.Bones.Num(); i++)
    {
        if (MeshData.Skeleton.Bones[i].ParentIndex == -1)
            RootBoneIndices.Add(i);
    }

    if (RootBoneIndices.Num() > 1)
    {
        FBone VirtualRoot;
        VirtualRoot.Name = "VirtualRoot";
        VirtualRoot.ParentIndex = -1;
        VirtualRoot.BindPose = FMatrix::Identity();
        VirtualRoot.InverseBindPose = FMatrix::Identity();

        MeshData.Skeleton.Bones.Insert(VirtualRoot, 0);

        // 모든 본 인덱스 +1
        for (int i = 1; i < MeshData.Skeleton.Bones.Num(); i++)
            MeshData.Skeleton.Bones[i].ParentIndex += 1;

        // 정점의 본 인덱스도 +1
        for (int i = 0; i < MeshData.Vertices.Num(); i++)
            for (int j = 0; j < 4; j++)
                MeshData.Vertices[i].BoneIndices[j] += 1;
    }
}
```

## 4. 스킨 가중치 처리

### 4.1 FBX 스킨 데이터 구조

FBX는 정점 중심이 아닌 **본 중심**으로 스킨 데이터를 저장합니다.

```
FbxSkin
 └─ FbxCluster (본 하나에 대응)
     ├─ GetLink() → FbxNode (본)
     ├─ GetControlPointIndices() → [10, 25, 47, ...] (이 본이 영향주는 정점들)
     ├─ GetControlPointWeights() → [0.8, 0.5, 0.3, ...] (각 정점에 대한 가중치)
     └─ GetTransformLinkMatrix() → Bind Pose 행렬
```

### 4.2 변환 과정

**1단계: 본→정점 매핑을 정점→본 매핑으로 전환**

```cpp
TMap<int32, TArray<IndexWeight>> ControlPointToBoneWeight;

for (각 클러스터)
{
    int BoneIndex = BoneToIndex[Cluster->GetLink()];
    int* CPIndices = Cluster->GetControlPointIndices();
    double* Weights = Cluster->GetControlPointWeights();

    for (int i = 0; i < Count; i++)
    {
        int CPIndex = CPIndices[i];
        ControlPointToBoneWeight[CPIndex].Add({BoneIndex, Weights[i]});
    }
}
```

**2단계: 정점 생성 시 본 인덱스/가중치 할당**

```cpp
int CPIndex = InMesh->GetPolygonVertex(PolyIdx, VertIdx);

if (ControlPointToBoneWeight.Contains(CPIndex))
{
    const TArray<IndexWeight>& Weights = ControlPointToBoneWeight[CPIndex];

    // 가중치 정규화
    double TotalWeight = 0.0;
    for (int i = 0; i < min(4, Weights.Num()); i++)
        TotalWeight += Weights[i].BoneWeight;

    // 최대 4개 본만 저장
    for (int i = 0; i < min(4, Weights.Num()); i++)
    {
        Vertex.BoneIndices[i] = Weights[i].BoneIndex;
        Vertex.BoneWeights[i] = Weights[i].BoneWeight / TotalWeight;
    }
}
```

## 5. 최적화 기법

### 5.1 캐싱 시스템
- FBX 파싱 결과를 `.bin` 파일로 캐싱 (라인 110-211)
- 타임스탬프 기반 자동 갱신
- 파싱 시간 대폭 절감

### 5.2 머티리얼 인덱싱
- 폴리곤 순회 전 머티리얼 슬롯→인덱스 매핑 생성 (라인 370)
- 폴리곤 순회 시 해싱 연산 제거

### 5.3 중복 정점 제거
- `TMap<FSkinnedVertex, uint32>` 사용 (라인 604)
- 완전히 동일한 정점만 공유

## 6. 제한 사항

1. **최대 4개 본 영향**: 정점당 최대 4개 본만 지원 (라인 640-651)
2. **CPU 스키닝**: GPU 스키닝 미구현
3. **애니메이션 미지원**: 애니메이션 커브/키프레임 임포트 없음
4. **Phong/Lambert만 지원**: PBR 머티리얼 미지원 (라인 1002)
