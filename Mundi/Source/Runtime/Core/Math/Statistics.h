#pragma once

#include "Vector.h"

// -------------------------------------------
// EDistributionMode - 분포 모드
// -------------------------------------------
enum class EDistributionMode : uint8
{
    Uniform,  // Min~Max 사이 랜덤 (기본값)
    Curve,     // 시간 기반 커브
    Count       //Enum갯수를 위한 것 Property에 보이지 않도록 할것
};

const static char* EDistributionModeNames[] =
{ "Uniform", "Curve" };

// -------------------------------------------
// FInterpCurvePoint - 키프레임 포인트
// -------------------------------------------
template <typename T>
struct FInterpCurvePoint
{
    float InVal{};  // 시간 (X축)
    T OutVal{};     // 값 (Y축)

    FInterpCurvePoint() = default;
    FInterpCurvePoint(float InTime, const T& InValue)
        : InVal(InTime), OutVal(InValue) {}
};

// -------------------------------------------
// FInterpCurve - 키프레임 기반 보간 커브
// -------------------------------------------
template <typename T>
struct FInterpCurve
{
    TArray<FInterpCurvePoint<T>> Points;

    // 키프레임 추가
    void AddPoint(float InTime, const T& InValue)
    {
        Points.Add(FInterpCurvePoint<T>(InTime, InValue));
        // 시간순 정렬
        std::sort(Points.begin(), Points.end(),
            [](const FInterpCurvePoint<T>& A, const FInterpCurvePoint<T>& B)
            {
                return A.InVal < B.InVal;
            });
    }

    // 시간에 따른 보간 값 반환
    T Eval(float InTime) const
    {
        if (Points.Num() == 0)
            return T{};

        if (Points.Num() == 1)
            return Points[0].OutVal;

        // 범위 밖 처리
        if (InTime <= Points[0].InVal)
            return Points[0].OutVal;

        if (InTime >= Points[Points.Num() - 1].InVal)
            return Points[Points.Num() - 1].OutVal;

        // 보간할 두 키프레임 찾기
        for (int32 i = 0; i < Points.Num() - 1; ++i)
        {
            if (InTime >= Points[i].InVal && InTime <= Points[i + 1].InVal)
            {
                float Alpha = (InTime - Points[i].InVal) / (Points[i + 1].InVal - Points[i].InVal);
                return FMath::Lerp(Points[i].OutVal, Points[i + 1].OutVal, Alpha);
            }
        }

        return Points[Points.Num() - 1].OutVal;
    }

    // 키프레임이 있는지 확인
    bool HasKeys() const { return Points.Num() > 0; }

    // 키프레임 개수 반환
    int32 Num() const { return Points.Num(); }

    // 모든 키프레임 제거
    void Reset() { Points.Empty(); }
};

// -------------------------------------------
// FRawDistribution - 기본 분포 템플릿
// -------------------------------------------
template <typename T>
struct FRawDistribution
{
    T Min{};
    T Max{};
    EDistributionMode Mode = EDistributionMode::Uniform;
    FInterpCurve<T> Curve;

    // 커브 시간 범위 (커브가 이 범위를 벗어나면 순환)
    float MinTime = 0.0f;
    float MaxTime = 1.0f;

    // 모드에 따른 값 반환
    T GetValue(float Time) const
    {
        if (Mode == EDistributionMode::Curve && Curve.HasKeys())
        {
            float NormalizedTime = NormalizeTime(Time);
            return Curve.Eval(NormalizedTime);
        }
        // 기본: Uniform 모드 (랜덤)
        return GetRandomValue();
    }

    // 단일 T 값으로 보간 (Uniform 모드용)
    T GetLerpValue(float T) const
    {
        return FMath::Lerp(Min, Max, T);
    }

    // 랜덤 값 반환 (기본 구현: 단일 랜덤 T 사용)
    T GetRandomValue() const
    {
        return FMath::Lerp(Min, Max, FMath::GetRandZeroOneRange());
    }

private:
    // 시간을 [MinTime, MaxTime] 범위로 정규화 (순환)
    float NormalizeTime(float Time) const
    {
        if (MaxTime <= MinTime)
            return MinTime;

        float Range = MaxTime - MinTime;
        float Offset = Time - MinTime;

        if (Offset < 0.0f)
            return MinTime;

        // MinTime~MaxTime 범위 내에서 시간을 순환
        return MinTime + fmodf(Offset, Range);
    }
};

// FVector 특수화: 각 축에 독립적인 랜덤 값 사용
template <>
struct FRawDistribution<FVector>
{
    FVector Min{};
    FVector Max{};
    EDistributionMode Mode = EDistributionMode::Uniform;
    FInterpCurve<FVector> Curve;

    // 커브 시간 범위 (커브가 이 범위를 벗어나면 순환)
    float MinTime = 0.0f;
    float MaxTime = 1.0f;

    // 모드에 따른 값 반환
    FVector GetValue(float Time) const
    {
        if (Mode == EDistributionMode::Curve && Curve.HasKeys())
        {
            float NormalizedTime = NormalizeTime(Time);
            return Curve.Eval(NormalizedTime);
        }
        // 기본: Uniform 모드 (랜덤)
        return GetRandomValue();
    }

    // 단일 T 값으로 보간 (Uniform 모드용)
    FVector GetLerpValue(float T) const
    {
        return FMath::Lerp(Min, Max, T);
    }

    // 각 축에 독립적인 랜덤 값 사용
    FVector GetRandomValue() const
    {
        return FVector(
            FMath::Lerp(Min.X, Max.X, FMath::GetRandZeroOneRange()),
            FMath::Lerp(Min.Y, Max.Y, FMath::GetRandZeroOneRange()),
            FMath::Lerp(Min.Z, Max.Z, FMath::GetRandZeroOneRange())
        );
    }

private:
    // 시간을 [MinTime, MaxTime] 범위로 정규화 (순환)
    float NormalizeTime(float Time) const
    {
        if (MaxTime <= MinTime)
            return MinTime;

        float Range = MaxTime - MinTime;
        float Offset = Time - MinTime;

        if (Offset < 0.0f)
            return MinTime;

        // MinTime~MaxTime 범위 내에서 시간을 순환
        return MinTime + fmodf(Offset, Range);
    }
};

