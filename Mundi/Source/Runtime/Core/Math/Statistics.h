#pragma once

#include "Vector.h"

template <typename T>
struct FRawDistribution
{
    T Min{};
    T Max{};

    // 단일 T 값으로 보간
    T GetValue(float T)
    {
        return FMath::Lerp(Min, Max, T);
    }

    // 랜덤 값 반환 (기본 구현: 단일 랜덤 T 사용)
    T GetRandomValue()
    {
        return FMath::Lerp(Min, Max, FMath::GetRandZeroOneRange());
    }
};

// FVector 특수화: 각 축에 독립적인 랜덤 값 사용
template <>
struct FRawDistribution<FVector>
{
    FVector Min{};
    FVector Max{};

    // 단일 T 값으로 보간
    FVector GetValue(float T)
    {
        return FMath::Lerp(Min, Max, T);
    }

    // 각 축에 독립적인 랜덤 값 사용
    FVector GetRandomValue()
    {
        return FVector(
            FMath::Lerp(Min.X, Max.X, FMath::GetRandZeroOneRange()),
            FMath::Lerp(Min.Y, Max.Y, FMath::GetRandZeroOneRange()),
            FMath::Lerp(Min.Z, Max.Z, FMath::GetRandZeroOneRange())
        );
    }
};

