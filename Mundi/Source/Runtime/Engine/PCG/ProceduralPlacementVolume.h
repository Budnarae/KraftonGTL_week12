#pragma once

#include "Actor.h"
#include "PlacementDistribution.h"
#include "MeshBVH.h"
#include "PlacementMeshEntry.h"
#include "AProceduralPlacementVolume.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UBillboardComponent;

// BVH 캐시 엔트리
struct FBVHCacheEntry
{
    FMeshBVH* BVH = nullptr;
    FStaticMesh* MeshAsset = nullptr;
    FMatrix WorldMatrix;
    FMatrix InvWorldMatrix;

    ~FBVHCacheEntry()
    {
        if (BVH)
        {
            delete BVH;
            BVH = nullptr;
        }
    }
};

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
    void Destroy() override;

    void Generate();
    void ClearPlacement();

    UBoxComponent* GetVolumeComponent() const { return VolumeComponent; }

protected:
    TArray<FVector> GeneratePoints();
    void SpawnMeshAtPoint(const FVector& LocalPosition, UPlacementMeshEntry* Entry);
    void SpawnMeshAtSurfacePoint(const FVector& WorldPosition, const FVector& SurfaceNormal, UPlacementMeshEntry* Entry);
    FTransform GenerateRandomTransform(const FVector& Position, UPlacementMeshEntry* Entry);
    FTransform GenerateRandomTransformWithNormal(const FVector& Position, const FVector& Normal, UPlacementMeshEntry* Entry);
    bool PassesDensityCheck(const FVector& Position);
    bool RaycastToSurface(const FVector& Origin, const FVector& Direction, FVector& OutHitPoint, FVector& OutHitNormal);

    // BVH 캐시 관리
    void BuildBVHCache();
    void ClearBVHCache();

    // 가중치 기반 메시 엔트리 선택
    UPlacementMeshEntry* SelectMeshEntryByWeight();

protected:
    UBoxComponent* VolumeComponent = nullptr;

    // 메시 배열 (에디터에서 동적으로 추가/삭제 가능, 각 엔트리에 스케일 설정 포함)
    UPROPERTY(EditAnywhere, Category="Placement|Mesh", Tooltip="Meshes with weights and scale settings for varied placement")
    TArray<UPlacementMeshEntry*> PlacementMeshes;

    // 가중치 합계 (캐싱)
    float TotalWeight = 0.0f;

    // BVH 캐시 (레이캐스트 성능 최적화)
    TMap<UStaticMeshComponent*, FBVHCacheEntry*> BVHCache;

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
    UPROPERTY(EditAnywhere, Category="Placement|Transform", Tooltip="Random yaw rotation")
    bool bRandomYaw = true;

    UPROPERTY(EditAnywhere, Category="Placement|Transform", Tooltip="Random pitch/roll tilt")
    bool bRandomTilt = true;

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

    // Surface placement
    UPROPERTY(EditAnywhere, Category="Placement|Surface", Tooltip="Place on mesh surfaces instead of volume")
    bool bPlaceOnSurface = false;

    UPROPERTY(EditAnywhere, Category="Placement|Surface", Tooltip="Align mesh rotation to surface normal")
    bool bAlignToNormal = true;

    UPROPERTY(EditAnywhere, Category="Placement|Surface", Tooltip="Raycast start height offset")
    float RaycastHeightOffset = 100.0f;

    // Control
    UPROPERTY(EditAnywhere, Category="Placement", Tooltip="Auto generate on BeginPlay")
    bool bGenerateOnBeginPlay = false;

    TArray<AActor*> SpawnedActors;
    FPlacementDistribution Distribution;

    UBillboardComponent* SpriteComponent = nullptr;

private:
    EDistributionType GetDistributionType() const;
};
