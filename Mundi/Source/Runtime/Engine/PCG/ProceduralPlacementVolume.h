#pragma once

#include "Actor.h"
#include "PlacementDistribution.h"
#include "AProceduralPlacementVolume.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UStaticMesh;

UCLASS(DisplayName="Procedural Placement Volume", Description="Procedurally places meshes within the volume")
class AProceduralPlacementVolume : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    AProceduralPlacementVolume();

protected:
    ~AProceduralPlacementVolume() override;

public:
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;

    void Generate();
    void ClearPlacement();

    UBoxComponent* GetVolumeComponent() const { return VolumeComponent; }

protected:
    TArray<FVector> GeneratePoints();
    void SpawnMeshAtPoint(const FVector& LocalPosition);
    FTransform GenerateRandomTransform(const FVector& Position);
    bool PassesDensityCheck(const FVector& Position);

protected:
    UBoxComponent* VolumeComponent = nullptr;

    // Mesh settings (simplified - single mesh)
    UPROPERTY(EditAnywhere, Category="Placement|Mesh", Tooltip="Mesh to place")
    UStaticMesh* PlacementMesh = nullptr;

    UPROPERTY(EditAnywhere, Category="Placement|Mesh", Tooltip="Minimum scale")
    FVector ScaleMin = FVector(0.8f, 0.8f, 0.8f);

    UPROPERTY(EditAnywhere, Category="Placement|Mesh", Tooltip="Maximum scale")
    FVector ScaleMax = FVector(1.2f, 1.2f, 1.2f);

    // Density settings
    UPROPERTY(EditAnywhere, Category="Placement|Density", Tooltip="Target placement count")
    int32 TargetCount = 100;

    UPROPERTY(EditAnywhere, Category="Placement|Density", Tooltip="Use noise-based density map")
    bool bUseDensityMap = false;

    UPROPERTY(EditAnywhere, Category="Placement|Density", Tooltip="Noise scale")
    float DensityNoiseScale = 0.01f;

    UPROPERTY(EditAnywhere, Category="Placement|Density", Tooltip="Density threshold (0~1)")
    float DensityThreshold = 0.3f;

    // Transform randomization
    UPROPERTY(EditAnywhere, Category="Placement|Transform", Tooltip="Uniform scale (use X only)")
    bool bUniformScale = true;

    UPROPERTY(EditAnywhere, Category="Placement|Transform", Tooltip="Random yaw rotation")
    bool bRandomYaw = true;

    UPROPERTY(EditAnywhere, Category="Placement|Transform", Tooltip="Max tilt angle (degrees)")
    float MaxTilt = 5.0f;

    // Distribution settings
    UPROPERTY(EditAnywhere, Category="Placement|Distribution", Tooltip="Distribution type (0=Random, 1=PoissonDisk, 2=Grid, 3=Clustered)")
    int32 DistributionTypeIndex = 1;

    UPROPERTY(EditAnywhere, Category="Placement|Distribution", Tooltip="Minimum distance (for Poisson Disk)")
    float MinDistance = 50.0f;

    // Seed
    UPROPERTY(EditAnywhere, Category="Placement|Seed", Tooltip="Random seed value")
    int32 RandomSeed = 12345;

    UPROPERTY(EditAnywhere, Category="Placement|Seed", Tooltip="Use random seed each time")
    bool bUseRandomSeed = false;

    // Control
    UPROPERTY(EditAnywhere, Category="Placement", Tooltip="Auto generate on BeginPlay")
    bool bGenerateOnBeginPlay = true;

    TArray<AActor*> SpawnedActors;
    FPlacementDistribution Distribution;

private:
    EDistributionType GetDistributionType() const;
};
