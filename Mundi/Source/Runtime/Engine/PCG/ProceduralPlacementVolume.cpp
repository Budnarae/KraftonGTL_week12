#include "pch.h"
#include "ProceduralPlacementVolume.h"
#include "NoiseGenerator.h"
#include "BoxComponent.h"
#include "StaticMeshComponent.h"
#include "World.h"
#include "StaticMesh.h"
#include "ObjectFactory.h"
#include <random>
#include <ctime>

IMPLEMENT_CLASS(AProceduralPlacementVolume)

AProceduralPlacementVolume::AProceduralPlacementVolume()
{
    bCanEverTick = false;

    VolumeComponent = CreateDefaultSubobject<UBoxComponent>(FName("VolumeExtent"));
    SetRootComponent(VolumeComponent);
    VolumeComponent->SetBoxExtent(FVector(500.0f, 500.0f, 100.0f));
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

    // 디버깅용 로그
    OutputDebugStringA(("PCG Generate - Extent: " + std::to_string(Extent.X) + ", " + std::to_string(Extent.Y) + ", " + std::to_string(Extent.Z) + "\n").c_str());
    OutputDebugStringA(("PCG Generate - TargetCount: " + std::to_string(TargetCount) + ", MinDistance: " + std::to_string(MinDistance) + "\n").c_str());

    TArray<FVector> Points = GeneratePoints();

    OutputDebugStringA(("PCG Generate - Points count: " + std::to_string(Points.Num()) + "\n").c_str());

    for (int32 i = 0; i < Points.Num(); ++i)
    {
        const FVector& Point = Points[i];

        OutputDebugStringA(("  Point[" + std::to_string(i) + "]: " + std::to_string(Point.X) + ", " + std::to_string(Point.Y) + ", " + std::to_string(Point.Z) + "\n").c_str());

        if (bUseDensityMap && !PassesDensityCheck(Point))
        {
            continue;
        }

        SpawnMeshAtPoint(Point);
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
