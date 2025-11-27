#include "pch.h"
#include "ProceduralPlacementVolume.h"
#include "NoiseGenerator.h"
#include "BoxComponent.h"
#include "StaticMeshComponent.h"
#include "BillboardComponent.h"
#include "World.h"
#include "StaticMesh.h"
#include "ObjectFactory.h"
#include "PathUtils.h"
#include "Picking.h"
#include "MeshBVH.h"
#include "VertexData.h"
#include <random>
#include <ctime>

IMPLEMENT_CLASS(AProceduralPlacementVolume)

AProceduralPlacementVolume::AProceduralPlacementVolume()
{
    bCanEverTick = false;

    VolumeComponent = CreateDefaultSubobject<UBoxComponent>(FName("VolumeExtent"));
    SetRootComponent(VolumeComponent);
    VolumeComponent->SetBoxExtent(FVector(10.0f, 10.0f, 2.0f));

    SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(FName("Sprite"));
    SpriteComponent->SetTexture(GDataDir + "/UI/Icons/EmptyActor.dds");
}

AProceduralPlacementVolume::~AProceduralPlacementVolume()
{
}

void AProceduralPlacementVolume::Destroy()
{
    // Volume 삭제 시 스폰된 액터들도 함께 삭제
    ClearPlacement();

    AActor::Destroy();
}

void AProceduralPlacementVolume::BeginPlay()
{
    AActor::BeginPlay();

    if (bGenerateOnBeginPlay)
    {
        Generate();
    }
}

void AProceduralPlacementVolume::Tick(float DeltaSeconds)
{
    AActor::Tick(DeltaSeconds);
}

EDistributionType AProceduralPlacementVolume::GetDistributionType() const
{
    switch (DistributionTypeIndex)
    {
    case 0: return EDistributionType::Random;
    case 1: return EDistributionType::PoissonDisk;
    case 2: return EDistributionType::Grid;
    case 3: return EDistributionType::Clustered;
    default: return EDistributionType::PoissonDisk;
    }
}

void AProceduralPlacementVolume::Generate()
{
    // 메시 배열 체크
    if (PlacementMeshes.Num() == 0)
    {
        ClearPlacement();
        return;
    }

    // 가중치 합계 계산
    TotalWeight = 0.0f;
    for (UPlacementMeshEntry* Entry : PlacementMeshes)
    {
        if (Entry && Entry->Mesh)
            TotalWeight += Entry->Weight;
    }

    if (TotalWeight <= 0.0f)
    {
        ClearPlacement();
        return;
    }

    int32 Seed = bUseRandomSeed ? static_cast<int32>(std::time(nullptr)) : RandomSeed;
    Distribution.SetSeed(Seed);
    FNoiseGenerator::Initialize(Seed);

    FVector Extent = VolumeComponent->GetBoxExtent();
    Distribution.SetBounds(Extent);

    // 표면 배치 모드면 BVH 캐시 먼저 빌드 (SpawnedActors로 이전 액터 제외 가능)
    // ClearPlacement 전에 빌드해야 SpawnedActors.Contains() 체크가 작동함
    if (bPlaceOnSurface)
    {
        BuildBVHCache();
    }

    // BVH 캐시 빌드 후 이전 액터들 삭제
    ClearPlacement();

    TArray<FVector> Points = GeneratePoints();

    // Surface 배치 모드에서 3D 최소 거리 체크용
    TArray<FVector> PlacedSurfacePoints;
    if (bPlaceOnSurface)
    {
        PlacedSurfacePoints.Reserve(Points.Num());
    }

    for (const FVector& Point : Points)
    {
        if (bUseDensityMap && !PassesDensityCheck(Point))
        {
            continue;
        }

        // 가중치 기반 메시 엔트리 선택
        UPlacementMeshEntry* SelectedEntry = SelectMeshEntryByWeight();
        if (!SelectedEntry || !SelectedEntry->Mesh)
        {
            continue;
        }

        if (bPlaceOnSurface)
        {
            // 표면 배치 모드: 위에서 아래로 레이캐스트
            FVector WorldXY = GetActorLocation() + Point;
            FVector RayOrigin = FVector(WorldXY.X, WorldXY.Y, GetActorLocation().Z + Extent.Z + RaycastHeightOffset);
            FVector RayDirection = FVector(0.0f, 0.0f, -1.0f);

            FVector HitPoint, HitNormal;
            if (RaycastToSurface(RayOrigin, RayDirection, HitPoint, HitNormal))
            {
                // 3D 공간에서 기존 배치점들과 최소 거리 체크
                bool bTooClose = false;
                for (const FVector& ExistingPoint : PlacedSurfacePoints)
                {
                    float Distance = (HitPoint - ExistingPoint).Size();
                    if (Distance < MinDistance)
                    {
                        bTooClose = true;
                        break;
                    }
                }

                if (!bTooClose)
                {
                    PlacedSurfacePoints.Add(HitPoint);
                    SpawnMeshAtSurfacePoint(HitPoint, HitNormal, SelectedEntry);
                }
            }
        }
        else
        {
            // 기존 볼륨 배치 모드
            SpawnMeshAtPoint(Point, SelectedEntry);
        }
    }

    // BVH 캐시 정리
    ClearBVHCache();
}

void AProceduralPlacementVolume::ClearPlacement()
{
    if (!World)
    {
        SpawnedActors.Empty();
        return;
    }

    // 현재 월드에 존재하는 액터 목록
    const TArray<AActor*>& WorldActors = World->GetActors();

    for (AActor* Actor : SpawnedActors)
    {
        // 액터가 여전히 월드에 존재하는지 확인 (외부 삭제 대응)
        if (Actor && WorldActors.Contains(Actor))
        {
            Actor->Destroy();
        }
    }
    SpawnedActors.Empty();
}

TArray<FVector> AProceduralPlacementVolume::GeneratePoints()
{
    // Surface 배치 모드 + Poisson Disk 분포일 때는 2D 버전 사용
    // (3D Poisson은 얇은 볼륨에서 포인트 생성이 제한됨)
    if (bPlaceOnSurface && GetDistributionType() == EDistributionType::PoissonDisk)
    {
        return Distribution.GeneratePoissonDisk2D(MinDistance, TargetCount);
    }
    return Distribution.GeneratePoints(GetDistributionType(), TargetCount, MinDistance);
}

void AProceduralPlacementVolume::SpawnMeshAtPoint(const FVector& LocalPosition, UPlacementMeshEntry* Entry)
{
    if (!World || !Entry || !Entry->Mesh)
        return;

    FTransform SpawnTransform = GenerateRandomTransform(LocalPosition, Entry);

    FVector WorldPos = GetActorLocation() + LocalPosition;
    SpawnTransform.Translation = WorldPos;

    AActor* SpawnedActor = World->SpawnActor(AActor::StaticClass(), SpawnTransform);
    if (SpawnedActor)
    {
        UStaticMeshComponent* MeshComp = SpawnedActor->CreateDefaultSubobject<UStaticMeshComponent>(FName("Mesh"));
        if (MeshComp)
        {
            SpawnedActor->SetRootComponent(MeshComp);
            MeshComp->SetStaticMesh(Entry->Mesh->GetAssetPathFileName());
        }

        SpawnedActor->SetActorLocation(SpawnTransform.Translation);
        SpawnedActor->SetActorScale(SpawnTransform.Scale3D);
        SpawnedActor->SetActorRotation(SpawnTransform.Rotation);

        SpawnedActors.Add(SpawnedActor);
    }
}

FTransform AProceduralPlacementVolume::GenerateRandomTransform(const FVector& Position, UPlacementMeshEntry* Entry)
{
    FTransform Transform;
    Transform.Translation = Position;

    // 엔트리의 스케일 설정 사용
    if (Entry->bRandomScale)
    {
        const FVector& ScaleMin = Entry->ScaleMin;
        const FVector& ScaleMax = Entry->ScaleMax;

        if (Entry->bUniformScale)
        {
            float UniformScaleValue = Distribution.RandomFloatInRange(ScaleMin.X, ScaleMax.X);
            Transform.Scale3D = FVector(UniformScaleValue, UniformScaleValue, UniformScaleValue);
        }
        else
        {
            Transform.Scale3D = FVector(
                Distribution.RandomFloatInRange(ScaleMin.X, ScaleMax.X),
                Distribution.RandomFloatInRange(ScaleMin.Y, ScaleMax.Y),
                Distribution.RandomFloatInRange(ScaleMin.Z, ScaleMax.Z)
            );
        }
    }
    else
    {
        Transform.Scale3D = FVector(1.0f, 1.0f, 1.0f);
    }

    float Yaw = bRandomYaw ? Distribution.RandomFloatInRange(0.0f, 360.0f) : 0.0f;
    float Pitch = bRandomTilt ? Distribution.RandomFloatInRange(-MaxTilt, MaxTilt) : 0.0f;
    float Roll = bRandomTilt ? Distribution.RandomFloatInRange(-MaxTilt, MaxTilt) : 0.0f;

    FQuat YawQuat = FQuat::FromAxisAngle(FVector(0, 0, 1), Yaw * (PI / 180.0f));
    FQuat PitchQuat = FQuat::FromAxisAngle(FVector(1, 0, 0), Pitch * (PI / 180.0f));
    FQuat RollQuat = FQuat::FromAxisAngle(FVector(0, 1, 0), Roll * (PI / 180.0f));

    Transform.Rotation = YawQuat * PitchQuat * RollQuat;

    return Transform;
}

bool AProceduralPlacementVolume::PassesDensityCheck(const FVector& Position)
{
    FVector WorldPos = GetActorLocation() + Position;

    float NoiseValue = FNoiseGenerator::NormalizedPerlin2D(
        WorldPos.X * DensityNoiseScale,
        WorldPos.Y * DensityNoiseScale
    );

    return NoiseValue > DensityThreshold;
}

bool AProceduralPlacementVolume::RaycastToSurface(const FVector& Origin, const FVector& Direction, FVector& OutHitPoint, FVector& OutHitNormal)
{
    if (!World)
        return false;

    float ClosestDistance = FLT_MAX;
    bool bHit = false;

    // 캐시된 BVH 사용
    for (auto& Pair : BVHCache)
    {
        FBVHCacheEntry* CacheEntry = Pair.second;
        if (!CacheEntry || !CacheEntry->BVH || !CacheEntry->MeshAsset)
            continue;

        // 캐시된 행렬 사용
        const FMatrix& WorldMatrix = CacheEntry->WorldMatrix;
        const FMatrix& InvWorld = CacheEntry->InvWorldMatrix;

        FVector4 RayOrigin4(Origin.X, Origin.Y, Origin.Z, 1.0f);
        FVector4 RayDir4(Direction.X, Direction.Y, Direction.Z, 0.0f);
        FVector4 LocalOrigin4 = RayOrigin4 * InvWorld;
        FVector4 LocalDir4 = RayDir4 * InvWorld;

        FRay LocalRay;
        LocalRay.Origin = FVector(LocalOrigin4.X, LocalOrigin4.Y, LocalOrigin4.Z);
        LocalRay.Direction = FVector(LocalDir4.X, LocalDir4.Y, LocalDir4.Z).GetNormalized();

        // 캐시된 BVH로 레이캐스트
        float HitDistance = 0.0f;
        FVector HitNormal;

        FStaticMesh* MeshAsset = CacheEntry->MeshAsset;
        if (CacheEntry->BVH->IntersectRayWithNormal(LocalRay, MeshAsset->Vertices, MeshAsset->Indices, HitDistance, HitNormal))
        {
            // 로컬 → 월드 변환
            FVector LocalHitPoint = LocalRay.Origin + LocalRay.Direction * HitDistance;
            FVector4 LocalHitPoint4(LocalHitPoint.X, LocalHitPoint.Y, LocalHitPoint.Z, 1.0f);
            FVector4 WorldHitPoint4 = LocalHitPoint4 * WorldMatrix;

            float WorldDistance = (FVector(WorldHitPoint4.X, WorldHitPoint4.Y, WorldHitPoint4.Z) - Origin).Size();

            if (WorldDistance < ClosestDistance)
            {
                // 노멀을 월드 공간으로 변환 (방향 벡터)
                FVector4 LocalNormal4(HitNormal.X, HitNormal.Y, HitNormal.Z, 0.0f);
                FMatrix NormalMatrix = InvWorld.Transpose();
                FVector4 WorldNormal4 = LocalNormal4 * NormalMatrix;
                FVector WorldNormal = FVector(WorldNormal4.X, WorldNormal4.Y, WorldNormal4.Z).GetNormalized();

                // 방어 코드: 월드 공간에서 노말이 레이와 같은 방향이면 무효한 히트
                // (정상적인 표면 히트라면 노말은 레이를 향해야 함 → 내적이 음수)
                if (FVector::Dot(WorldNormal, Direction) > 0.0f)
                {
                    continue;  // 이 히트 무시, 다음 메시 검사
                }

                ClosestDistance = WorldDistance;
                OutHitPoint = FVector(WorldHitPoint4.X, WorldHitPoint4.Y, WorldHitPoint4.Z);
                OutHitNormal = WorldNormal;
                bHit = true;
            }
        }
    }

    return bHit;
}

void AProceduralPlacementVolume::SpawnMeshAtSurfacePoint(const FVector& WorldPosition, const FVector& SurfaceNormal, UPlacementMeshEntry* Entry)
{
    if (!World || !Entry || !Entry->Mesh)
        return;

    FTransform SpawnTransform = GenerateRandomTransformWithNormal(WorldPosition, SurfaceNormal, Entry);

    AActor* SpawnedActor = World->SpawnActor(AActor::StaticClass(), SpawnTransform);
    if (SpawnedActor)
    {
        UStaticMeshComponent* MeshComp = SpawnedActor->CreateDefaultSubobject<UStaticMeshComponent>(FName("Mesh"));
        if (MeshComp)
        {
            SpawnedActor->SetRootComponent(MeshComp);
            MeshComp->SetStaticMesh(Entry->Mesh->GetAssetPathFileName());
        }

        SpawnedActor->SetActorLocation(SpawnTransform.Translation);
        SpawnedActor->SetActorScale(SpawnTransform.Scale3D);
        SpawnedActor->SetActorRotation(SpawnTransform.Rotation);

        SpawnedActors.Add(SpawnedActor);
    }
}

FTransform AProceduralPlacementVolume::GenerateRandomTransformWithNormal(const FVector& Position, const FVector& Normal, UPlacementMeshEntry* Entry)
{
    FTransform Transform;
    Transform.Translation = Position;

    // 엔트리의 스케일 설정 사용
    if (Entry->bRandomScale)
    {
        const FVector& ScaleMin = Entry->ScaleMin;
        const FVector& ScaleMax = Entry->ScaleMax;

        if (Entry->bUniformScale)
        {
            float UniformScaleValue = Distribution.RandomFloatInRange(ScaleMin.X, ScaleMax.X);
            Transform.Scale3D = FVector(UniformScaleValue, UniformScaleValue, UniformScaleValue);
        }
        else
        {
            Transform.Scale3D = FVector(
                Distribution.RandomFloatInRange(ScaleMin.X, ScaleMax.X),
                Distribution.RandomFloatInRange(ScaleMin.Y, ScaleMax.Y),
                Distribution.RandomFloatInRange(ScaleMin.Z, ScaleMax.Z)
            );
        }
    }
    else
    {
        Transform.Scale3D = FVector(1.0f, 1.0f, 1.0f);
    }

    // 노멀 정렬 회전 계산
    FQuat AlignRotation = FQuat::Identity();
    if (bAlignToNormal)
    {
        FVector Up(0.0f, 0.0f, 1.0f);
        FVector Axis = FVector::Cross(Up, Normal);
        float Dot = FVector::Dot(Up, Normal);

        if (Axis.SizeSquared() > 0.0001f)
        {
            float Angle = std::acos(std::clamp(Dot, -1.0f, 1.0f));
            AlignRotation = FQuat::FromAxisAngle(Axis.GetNormalized(), Angle);
        }
        else if (Dot < 0)
        {
            // 완전히 반대 방향인 경우
            AlignRotation = FQuat::FromAxisAngle(FVector(1, 0, 0), PI);
        }
    }

    // 랜덤 Yaw 회전 (정렬된 후 Normal 축 기준)
    float Yaw = bRandomYaw ? Distribution.RandomFloatInRange(0.0f, 360.0f) : 0.0f;
    FQuat YawQuat = FQuat::FromAxisAngle(Normal, Yaw * (PI / 180.0f));

    // 최종 회전 = Yaw * 노멀 정렬 (Q1 * Q2 = Q2 먼저 적용, Q1 나중)
    // 즉, AlignRotation 먼저 적용 → 그 다음 YawQuat 적용
    Transform.Rotation = YawQuat * AlignRotation;

    return Transform;
}

void AProceduralPlacementVolume::BuildBVHCache()
{
    ClearBVHCache();

    if (!World)
        return;

    TArray<AActor*> Actors = World->GetActors();
    for (AActor* Actor : Actors)
    {
        if (!Actor || Actor == this)
            continue;

        // 이미 스폰한 액터는 제외
        if (SpawnedActors.Contains(Actor))
            continue;

        UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Actor->GetComponent(UStaticMeshComponent::StaticClass()));
        if (!MeshComp)
            continue;

        UStaticMesh* Mesh = MeshComp->GetStaticMesh();
        if (!Mesh)
            continue;

        FStaticMesh* MeshAsset = Mesh->GetStaticMeshAsset();
        if (!MeshAsset)
            continue;

        // BVH 캐시 엔트리 생성
        FBVHCacheEntry* CacheEntry = new FBVHCacheEntry();
        CacheEntry->WorldMatrix = Actor->GetActorTransform().ToMatrix();
        CacheEntry->InvWorldMatrix = CacheEntry->WorldMatrix.InverseAffine();
        CacheEntry->MeshAsset = MeshAsset;
        CacheEntry->BVH = new FMeshBVH();
        CacheEntry->BVH->Build(MeshAsset->Vertices, MeshAsset->Indices);

        BVHCache.Add(MeshComp, CacheEntry);
    }
}

void AProceduralPlacementVolume::ClearBVHCache()
{
    for (auto& Pair : BVHCache)
    {
        if (Pair.second)
        {
            delete Pair.second;
        }
    }
    BVHCache.Empty();
}

UPlacementMeshEntry* AProceduralPlacementVolume::SelectMeshEntryByWeight()
{
    if (PlacementMeshes.Num() == 0 || TotalWeight <= 0.0f)
    {
        return nullptr;
    }

    // 가중치 기반 랜덤 선택
    float RandomValue = Distribution.RandomFloatInRange(0.0f, TotalWeight);
    float AccumulatedWeight = 0.0f;

    for (UPlacementMeshEntry* Entry : PlacementMeshes)
    {
        if (!Entry || !Entry->Mesh)
            continue;

        AccumulatedWeight += Entry->Weight;
        if (RandomValue <= AccumulatedWeight)
        {
            return Entry;
        }
    }

    // 폴백: 마지막 유효한 엔트리
    for (int32 i = PlacementMeshes.Num() - 1; i >= 0; --i)
    {
        if (PlacementMeshes[i] && PlacementMeshes[i]->Mesh)
            return PlacementMeshes[i];
    }

    return nullptr;
}