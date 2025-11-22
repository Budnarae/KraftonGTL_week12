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
    ClearPlacement();

    if (!PlacementMesh)
    {
        return;
    }

    int32 Seed = bUseRandomSeed ? static_cast<int32>(std::time(nullptr)) : RandomSeed;
    Distribution.SetSeed(Seed);
    FNoiseGenerator::Initialize(Seed);

    FVector Extent = VolumeComponent->GetBoxExtent();
    Distribution.SetBounds(Extent);

    TArray<FVector> Points = GeneratePoints();

    for (const FVector& Point : Points)
    {
        if (bUseDensityMap && !PassesDensityCheck(Point))
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
                SpawnMeshAtSurfacePoint(HitPoint, HitNormal);
            }
        }
        else
        {
            // 기존 볼륨 배치 모드
            SpawnMeshAtPoint(Point);
        }
    }
}

void AProceduralPlacementVolume::ClearPlacement()
{
    for (AActor* Actor : SpawnedActors)
    {
        if (Actor)
        {
            Actor->Destroy();
        }
    }
    SpawnedActors.Empty();
}

TArray<FVector> AProceduralPlacementVolume::GeneratePoints()
{
    return Distribution.GeneratePoints(GetDistributionType(), TargetCount, MinDistance);
}

void AProceduralPlacementVolume::SpawnMeshAtPoint(const FVector& LocalPosition)
{
    if (!World || !PlacementMesh)
    {
        return;
    }

    FTransform SpawnTransform = GenerateRandomTransform(LocalPosition);

    FVector WorldPos = GetActorLocation() + LocalPosition;
    SpawnTransform.Translation = WorldPos;

    AActor* SpawnedActor = World->SpawnActor(AActor::StaticClass(), SpawnTransform);
    if (SpawnedActor)
    {
        UStaticMeshComponent* MeshComp = SpawnedActor->CreateDefaultSubobject<UStaticMeshComponent>(FName("Mesh"));
        if (MeshComp)
        {
            SpawnedActor->SetRootComponent(MeshComp);
            MeshComp->SetStaticMesh(PlacementMesh->GetAssetPathFileName());
        }

        SpawnedActor->SetActorLocation(SpawnTransform.Translation);
        SpawnedActor->SetActorScale(SpawnTransform.Scale3D);
        SpawnedActor->SetActorRotation(SpawnTransform.Rotation);

        SpawnedActors.Add(SpawnedActor);
    }
}

FTransform AProceduralPlacementVolume::GenerateRandomTransform(const FVector& Position)
{
    FTransform Transform;
    Transform.Translation = Position;

    if (bUniformScale)
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

    float Yaw = bRandomYaw ? Distribution.RandomFloatInRange(0.0f, 360.0f) : 0.0f;
    float Pitch = Distribution.RandomFloatInRange(-MaxTilt, MaxTilt);
    float Roll = Distribution.RandomFloatInRange(-MaxTilt, MaxTilt);

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

    TArray<AActor*> Actors = World->GetActors();
    for (AActor* Actor : Actors)
    {
        if (!Actor || Actor == this)
            continue;

        // StaticMeshComponent 찾기
        UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Actor->GetComponent(UStaticMeshComponent::StaticClass()));
        if (!MeshComp)
            continue;

        UStaticMesh* Mesh = MeshComp->GetStaticMesh();
        if (!Mesh)
            continue;

        FStaticMesh* MeshAsset = Mesh->GetStaticMeshAsset();
        if (!MeshAsset)
            continue;

        // 월드 → 로컬 변환
        FMatrix WorldMatrix = Actor->GetActorTransform().ToMatrix();
        FMatrix InvWorld = WorldMatrix.InverseAffine();

        FVector4 RayOrigin4(Origin.X, Origin.Y, Origin.Z, 1.0f);
        FVector4 RayDir4(Direction.X, Direction.Y, Direction.Z, 0.0f);
        FVector4 LocalOrigin4 = RayOrigin4 * InvWorld;
        FVector4 LocalDir4 = RayDir4 * InvWorld;

        FRay LocalRay;
        LocalRay.Origin = FVector(LocalOrigin4.X, LocalOrigin4.Y, LocalOrigin4.Z);
        LocalRay.Direction = FVector(LocalDir4.X, LocalDir4.Y, LocalDir4.Z).GetNormalized();

        // BVH 빌드 및 레이캐스트
        float HitDistance = 0.0f;
        FVector HitNormal;

        FMeshBVH BVH;
        BVH.Build(MeshAsset->Vertices, MeshAsset->Indices);

        if (BVH.IntersectRayWithNormal(LocalRay, MeshAsset->Vertices, MeshAsset->Indices, HitDistance, HitNormal))
        {
            // 로컬 → 월드 변환
            FVector LocalHitPoint = LocalRay.Origin + LocalRay.Direction * HitDistance;
            FVector4 LocalHitPoint4(LocalHitPoint.X, LocalHitPoint.Y, LocalHitPoint.Z, 1.0f);
            FVector4 WorldHitPoint4 = LocalHitPoint4 * WorldMatrix;

            float WorldDistance = (FVector(WorldHitPoint4.X, WorldHitPoint4.Y, WorldHitPoint4.Z) - Origin).Size();

            if (WorldDistance < ClosestDistance)
            {
                ClosestDistance = WorldDistance;
                OutHitPoint = FVector(WorldHitPoint4.X, WorldHitPoint4.Y, WorldHitPoint4.Z);

                // 노멀도 월드 공간으로 변환 (방향 벡터)
                FVector4 LocalNormal4(HitNormal.X, HitNormal.Y, HitNormal.Z, 0.0f);
                FMatrix NormalMatrix = InvWorld.Transpose();
                FVector4 WorldNormal4 = LocalNormal4 * NormalMatrix;
                OutHitNormal = FVector(WorldNormal4.X, WorldNormal4.Y, WorldNormal4.Z).GetNormalized();

                bHit = true;
            }
        }
    }

    return bHit;
}

void AProceduralPlacementVolume::SpawnMeshAtSurfacePoint(const FVector& WorldPosition, const FVector& SurfaceNormal)
{
    if (!World || !PlacementMesh)
        return;

    FTransform SpawnTransform = GenerateRandomTransformWithNormal(WorldPosition, SurfaceNormal);

    AActor* SpawnedActor = World->SpawnActor(AActor::StaticClass(), SpawnTransform);
    if (SpawnedActor)
    {
        UStaticMeshComponent* MeshComp = SpawnedActor->CreateDefaultSubobject<UStaticMeshComponent>(FName("Mesh"));
        if (MeshComp)
        {
            SpawnedActor->SetRootComponent(MeshComp);
            MeshComp->SetStaticMesh(PlacementMesh->GetAssetPathFileName());
        }

        SpawnedActor->SetActorLocation(SpawnTransform.Translation);
        SpawnedActor->SetActorScale(SpawnTransform.Scale3D);
        SpawnedActor->SetActorRotation(SpawnTransform.Rotation);

        SpawnedActors.Add(SpawnedActor);
    }
}

FTransform AProceduralPlacementVolume::GenerateRandomTransformWithNormal(const FVector& Position, const FVector& Normal)
{
    FTransform Transform;
    Transform.Translation = Position;

    // 스케일 설정
    if (bUniformScale)
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

    // 랜덤 Yaw 회전
    float Yaw = bRandomYaw ? Distribution.RandomFloatInRange(0.0f, 360.0f) : 0.0f;
    FQuat YawQuat = FQuat::FromAxisAngle(Normal, Yaw * (PI / 180.0f));

    // 최종 회전 = 노멀 정렬 * Yaw
    Transform.Rotation = AlignRotation * YawQuat;

    return Transform;
}