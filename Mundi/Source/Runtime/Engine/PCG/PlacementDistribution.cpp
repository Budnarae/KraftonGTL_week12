#include "pch.h"
#include "PlacementDistribution.h"
#include <cmath>

FPlacementDistribution::FPlacementDistribution()
    : Extent(100.0f, 100.0f, 100.0f)
    , Rng(12345)
    , Dist01(0.0f, 1.0f)
{
}

void FPlacementDistribution::SetSeed(int32 Seed)
{
    Rng.seed(Seed);
}

void FPlacementDistribution::SetBounds(const FVector& InExtent)
{
    Extent = InExtent;
}

float FPlacementDistribution::RandomFloat()
{
    return Dist01(Rng);
}

float FPlacementDistribution::RandomFloatInRange(float Min, float Max)
{
    return Min + RandomFloat() * (Max - Min);
}

int32 FPlacementDistribution::RandomInt(int32 Min, int32 Max)
{
    std::uniform_int_distribution<int32> Dist(Min, Max);
    return Dist(Rng);
}

FVector FPlacementDistribution::RandomPointInBounds()
{
    return FVector(
        RandomFloatInRange(-Extent.X, Extent.X),
        RandomFloatInRange(-Extent.Y, Extent.Y),
        RandomFloatInRange(-Extent.Z, Extent.Z)
    );
}

TArray<FVector> FPlacementDistribution::GeneratePoints(EDistributionType Type, int32 Count, float MinDistance)
{
    switch (Type)
    {
    case EDistributionType::Random:
        return GenerateRandom(Count);
    case EDistributionType::PoissonDisk:
        return GeneratePoissonDisk(MinDistance, Count);
    case EDistributionType::Grid:
        return GenerateGrid(Count);
    case EDistributionType::Clustered:
        return GenerateClustered(Count / 10 + 1, 10, MinDistance * 2.0f);
    default:
        return GenerateRandom(Count);
    }
}

TArray<FVector> FPlacementDistribution::GenerateRandom(int32 Count)
{
    TArray<FVector> Points;
    Points.Reserve(Count);

    for (int32 i = 0; i < Count; ++i)
    {
        Points.Add(RandomPointInBounds());
    }

    return Points;
}

TArray<FVector> FPlacementDistribution::GeneratePoissonDisk(float MinDistance, int32 MaxPoints)
{
    TArray<FVector> Points;
    TArray<int32> ActiveList;

    float CellSize = MinDistance / 1.732f;

    int32 GridSizeX = static_cast<int32>(std::ceil(Extent.X * 2.0f / CellSize));
    int32 GridSizeY = static_cast<int32>(std::ceil(Extent.Y * 2.0f / CellSize));
    int32 GridSizeZ = static_cast<int32>(std::ceil(Extent.Z * 2.0f / CellSize));

    int32 TotalCells = GridSizeX * GridSizeY * GridSizeZ;
    TArray<int32> Grid;
    Grid.SetNum(TotalCells);
    for (int32 i = 0; i < TotalCells; ++i)
    {
        Grid[i] = -1;
    }

    auto GetGridIndex = [GridSizeX, GridSizeY, GridSizeZ](int32 x, int32 y, int32 z) -> int32
    {
        if (x < 0 || x >= GridSizeX || y < 0 || y >= GridSizeY || z < 0 || z >= GridSizeZ)
            return -1;
        return x + y * GridSizeX + z * GridSizeX * GridSizeY;
    };

    FVector FirstPoint = RandomPointInBounds();
    Points.Add(FirstPoint);
    ActiveList.Add(0);

    {
        int32 cx = static_cast<int32>((FirstPoint.X + Extent.X) / CellSize);
        int32 cy = static_cast<int32>((FirstPoint.Y + Extent.Y) / CellSize);
        int32 cz = static_cast<int32>((FirstPoint.Z + Extent.Z) / CellSize);
        int32 idx = GetGridIndex(cx, cy, cz);
        if (idx >= 0) Grid[idx] = 0;
    }

    const int32 MaxAttempts = 30;

    while (ActiveList.Num() > 0 && Points.Num() < MaxPoints)
    {
        int32 ActiveIndex = RandomInt(0, ActiveList.Num() - 1);
        int32 PointIndex = ActiveList[ActiveIndex];
        FVector Point = Points[PointIndex];

        bool bFound = false;

        for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
        {
            float Radius = RandomFloatInRange(MinDistance, MinDistance * 2.0f);
            float Theta = RandomFloatInRange(0.0f, TWO_PI);
            float Phi = RandomFloatInRange(0.0f, PI);

            FVector NewPoint = Point + FVector(
                Radius * std::sin(Phi) * std::cos(Theta),
                Radius * std::sin(Phi) * std::sin(Theta),
                Radius * std::cos(Phi)
            );

            if (NewPoint.X < -Extent.X || NewPoint.X > Extent.X ||
                NewPoint.Y < -Extent.Y || NewPoint.Y > Extent.Y ||
                NewPoint.Z < -Extent.Z || NewPoint.Z > Extent.Z)
            {
                continue;
            }

            int32 CellX = static_cast<int32>((NewPoint.X + Extent.X) / CellSize);
            int32 CellY = static_cast<int32>((NewPoint.Y + Extent.Y) / CellSize);
            int32 CellZ = static_cast<int32>((NewPoint.Z + Extent.Z) / CellSize);

            bool bValid = true;
            for (int32 dx = -2; dx <= 2 && bValid; ++dx)
            {
                for (int32 dy = -2; dy <= 2 && bValid; ++dy)
                {
                    for (int32 dz = -2; dz <= 2 && bValid; ++dz)
                    {
                        int32 idx = GetGridIndex(CellX + dx, CellY + dy, CellZ + dz);
                        if (idx >= 0 && Grid[idx] != -1)
                        {
                            float DistSq = (Points[Grid[idx]] - NewPoint).SizeSquared();
                            if (DistSq < MinDistance * MinDistance)
                            {
                                bValid = false;
                            }
                        }
                    }
                }
            }

            if (bValid)
            {
                int32 NewIndex = Points.Num();
                Points.Add(NewPoint);
                ActiveList.Add(NewIndex);

                int32 idx = GetGridIndex(CellX, CellY, CellZ);
                if (idx >= 0) Grid[idx] = NewIndex;

                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            ActiveList.RemoveAt(ActiveIndex);
        }
    }

    return Points;
}

TArray<FVector> FPlacementDistribution::GeneratePoissonDisk2D(float MinDistance, int32 MaxPoints)
{
    TArray<FVector> Points;
    TArray<int32> ActiveList;

    float CellSize = MinDistance / 1.414f;  // sqrt(2) for 2D

    int32 GridSizeX = static_cast<int32>(std::ceil(Extent.X * 2.0f / CellSize));
    int32 GridSizeY = static_cast<int32>(std::ceil(Extent.Y * 2.0f / CellSize));

    int32 TotalCells = GridSizeX * GridSizeY;
    TArray<int32> Grid;
    Grid.SetNum(TotalCells);
    for (int32 i = 0; i < TotalCells; ++i)
    {
        Grid[i] = -1;
    }

    auto GetGridIndex = [GridSizeX, GridSizeY](int32 x, int32 y) -> int32
    {
        if (x < 0 || x >= GridSizeX || y < 0 || y >= GridSizeY)
            return -1;
        return x + y * GridSizeX;
    };

    // 첫 포인트 (Z는 0으로 설정, 나중에 레이캐스트로 결정됨)
    FVector FirstPoint(
        RandomFloatInRange(-Extent.X, Extent.X),
        RandomFloatInRange(-Extent.Y, Extent.Y),
        0.0f
    );
    Points.Add(FirstPoint);
    ActiveList.Add(0);

    {
        int32 cx = static_cast<int32>((FirstPoint.X + Extent.X) / CellSize);
        int32 cy = static_cast<int32>((FirstPoint.Y + Extent.Y) / CellSize);
        int32 idx = GetGridIndex(cx, cy);
        if (idx >= 0) Grid[idx] = 0;
    }

    const int32 MaxAttempts = 30;

    while (ActiveList.Num() > 0 && Points.Num() < MaxPoints)
    {
        int32 ActiveIndex = RandomInt(0, ActiveList.Num() - 1);
        int32 PointIndex = ActiveList[ActiveIndex];
        FVector Point = Points[PointIndex];

        bool bFound = false;

        for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
        {
            // 2D 원형 샘플링 (XY 평면)
            float Radius = RandomFloatInRange(MinDistance, MinDistance * 2.0f);
            float Theta = RandomFloatInRange(0.0f, TWO_PI);

            FVector NewPoint(
                Point.X + Radius * std::cos(Theta),
                Point.Y + Radius * std::sin(Theta),
                0.0f  // Z는 0 (레이캐스트에서 결정)
            );

            // XY 범위 체크
            if (NewPoint.X < -Extent.X || NewPoint.X > Extent.X ||
                NewPoint.Y < -Extent.Y || NewPoint.Y > Extent.Y)
            {
                continue;
            }

            int32 CellX = static_cast<int32>((NewPoint.X + Extent.X) / CellSize);
            int32 CellY = static_cast<int32>((NewPoint.Y + Extent.Y) / CellSize);

            // 2D 거리 체크 (주변 셀만)
            bool bValid = true;
            for (int32 dx = -2; dx <= 2 && bValid; ++dx)
            {
                for (int32 dy = -2; dy <= 2 && bValid; ++dy)
                {
                    int32 idx = GetGridIndex(CellX + dx, CellY + dy);
                    if (idx >= 0 && Grid[idx] != -1)
                    {
                        // 2D 거리만 체크 (XY)
                        FVector Diff = Points[Grid[idx]] - NewPoint;
                        float DistSq = Diff.X * Diff.X + Diff.Y * Diff.Y;
                        if (DistSq < MinDistance * MinDistance)
                        {
                            bValid = false;
                        }
                    }
                }
            }

            if (bValid)
            {
                int32 NewIndex = Points.Num();
                Points.Add(NewPoint);
                ActiveList.Add(NewIndex);

                int32 idx = GetGridIndex(CellX, CellY);
                if (idx >= 0) Grid[idx] = NewIndex;

                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            ActiveList.RemoveAt(ActiveIndex);
        }
    }

    return Points;
}

TArray<FVector> FPlacementDistribution::GenerateGrid(int32 Count, float Jitter)
{
    TArray<FVector> Points;

    float Volume = 8.0f * Extent.X * Extent.Y * Extent.Z;
    float PointsPerUnit = std::cbrt(static_cast<float>(Count) / Volume);

    int32 CountX = FMath::Max(1, static_cast<int32>(Extent.X * 2.0f * PointsPerUnit));
    int32 CountY = FMath::Max(1, static_cast<int32>(Extent.Y * 2.0f * PointsPerUnit));
    int32 CountZ = FMath::Max(1, static_cast<int32>(Extent.Z * 2.0f * PointsPerUnit));

    float SpacingX = Extent.X * 2.0f / CountX;
    float SpacingY = Extent.Y * 2.0f / CountY;
    float SpacingZ = Extent.Z * 2.0f / CountZ;

    for (int32 x = 0; x < CountX; ++x)
    {
        for (int32 y = 0; y < CountY; ++y)
        {
            for (int32 z = 0; z < CountZ; ++z)
            {
                FVector BasePos(
                    -Extent.X + SpacingX * (x + 0.5f),
                    -Extent.Y + SpacingY * (y + 0.5f),
                    -Extent.Z + SpacingZ * (z + 0.5f)
                );

                FVector JitterOffset(
                    RandomFloatInRange(-SpacingX * Jitter, SpacingX * Jitter),
                    RandomFloatInRange(-SpacingY * Jitter, SpacingY * Jitter),
                    RandomFloatInRange(-SpacingZ * Jitter, SpacingZ * Jitter)
                );

                FVector FinalPos = BasePos + JitterOffset;

                FinalPos.X = FMath::Clamp(FinalPos.X, -Extent.X, Extent.X);
                FinalPos.Y = FMath::Clamp(FinalPos.Y, -Extent.Y, Extent.Y);
                FinalPos.Z = FMath::Clamp(FinalPos.Z, -Extent.Z, Extent.Z);

                Points.Add(FinalPos);
            }
        }
    }

    return Points;
}

TArray<FVector> FPlacementDistribution::GenerateClustered(int32 ClusterCount, int32 PointsPerCluster, float ClusterRadius)
{
    TArray<FVector> Points;

    for (int32 c = 0; c < ClusterCount; ++c)
    {
        FVector ClusterCenter = RandomPointInBounds();

        for (int32 p = 0; p < PointsPerCluster; ++p)
        {
            float Radius = RandomFloat() * ClusterRadius;
            float Theta = RandomFloatInRange(0.0f, TWO_PI);
            float Phi = RandomFloatInRange(0.0f, PI);

            FVector Offset(
                Radius * std::sin(Phi) * std::cos(Theta),
                Radius * std::sin(Phi) * std::sin(Theta),
                Radius * std::cos(Phi)
            );

            FVector Point = ClusterCenter + Offset;

            Point.X = FMath::Clamp(Point.X, -Extent.X, Extent.X);
            Point.Y = FMath::Clamp(Point.Y, -Extent.Y, Extent.Y);
            Point.Z = FMath::Clamp(Point.Z, -Extent.Z, Extent.Z);

            Points.Add(Point);
        }
    }

    return Points;
}
