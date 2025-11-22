#pragma once

#include "Vector.h"
#include <random>

enum class EDistributionType : uint8
{
    Random,
    PoissonDisk,
    Grid,
    Clustered
};

class FPlacementDistribution
{
public:
    FPlacementDistribution();

    void SetSeed(int32 Seed);
    void SetBounds(const FVector& InExtent);

    TArray<FVector> GeneratePoints(EDistributionType Type, int32 Count, float MinDistance = 0.0f);

    TArray<FVector> GenerateRandom(int32 Count);
    TArray<FVector> GeneratePoissonDisk(float MinDistance, int32 MaxPoints = 10000);
    TArray<FVector> GenerateGrid(int32 Count, float Jitter = 0.3f);
    TArray<FVector> GenerateClustered(int32 ClusterCount, int32 PointsPerCluster, float ClusterRadius);

    float RandomFloat();
    float RandomFloatInRange(float Min, float Max);
    FVector RandomPointInBounds();
    int32 RandomInt(int32 Min, int32 Max);

private:
    FVector Extent;
    std::mt19937 Rng;
    std::uniform_real_distribution<float> Dist01;
};
