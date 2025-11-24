#include "pch.h"
#include "ParticleModuleTypeDataBeam.h"
#include "ParticleData.h"
#include "Source/Runtime/Engine/PCG/NoiseGenerator.h"
#include <cmath>

UParticleModuleTypeDataBeam::UParticleModuleTypeDataBeam()
    : UParticleModuleTypeDataBase()
{
}

void UParticleModuleTypeDataBeam::Spawn(FParticleContext& Context, float EmitterTime)
{
    // Beam particles typically don't need per-particle spawn logic
    // The beam is defined by the module settings
}

void UParticleModuleTypeDataBeam::Update(FParticleContext& Context, float DeltaTime)
{
    // Beam can be animated here if needed
    // For example, animating noise over time
}

void UParticleModuleTypeDataBeam::CalculateBeamPoints(
    const FVector& EmitterLocation,
    TArray<FVector>& OutPoints,
    TArray<float>& OutWidths,
    float Time
) const
{
    OutPoints.Empty();
    OutWidths.Empty();

    if (SegmentCount <= 0)
        return;

    // Calculate start and end points based on beam method
    FVector StartPoint = EmitterLocation + SourcePoint;
    FVector EndPoint;

    switch (BeamMethod)
    {
    case EBeamMethod::Distance:
        EndPoint = StartPoint + BeamDirection.GetNormalized() * BeamLength;
        break;
    case EBeamMethod::Target:
        EndPoint = TargetPoint;
        break;
    case EBeamMethod::Source:
    default:
        EndPoint = StartPoint + FVector(BeamLength, 0.0f, 0.0f);
        break;
    }

    // 노이즈가 있으면 Midpoint Displacement 사용, 없으면 직선
    if (NoiseAmplitude > 0.0f)
    {
        // SegmentCount를 depth로 변환 (depth=4 -> 16 세그먼트)
        // log2(SegmentCount)를 depth로 사용
        int32 Depth = 1;
        int32 Temp = SegmentCount;
        while (Temp > 1)
        {
            Temp >>= 1;
            Depth++;
        }
        Depth = std::min(Depth, 6);  // 최대 depth 6 (64 세그먼트)

        // 시간을 양자화하여 지직거리는 효과
        int32 TimeSeed = static_cast<int32>(Time * JitterFrequency);  // JitterFrequency에 따라 변화

        // Midpoint Displacement로 번개 포인트 생성
        FNoiseGenerator::GenerateLightningPoints(
            StartPoint,
            EndPoint,
            Depth,
            NoiseAmplitude,
            TimeSeed,
            OutPoints,
            DisplacementDecay
        );
    }
    else
    {
        // 노이즈 없으면 직선
        int32 NumPoints = SegmentCount + 1;
        OutPoints.Reserve(NumPoints);

        for (int32 i = 0; i < NumPoints; ++i)
        {
            float T = static_cast<float>(i) / static_cast<float>(SegmentCount);
            FVector Point = StartPoint + (EndPoint - StartPoint) * T;
            OutPoints.Add(Point);
        }
    }

    // Width 계산
    OutWidths.Reserve(OutPoints.Num());
    for (int32 i = 0; i < OutPoints.Num(); ++i)
    {
        float T = static_cast<float>(i) / static_cast<float>(OutPoints.Num() - 1);

        float Width = BeamWidth;
        if (bTaperBeam)
        {
            // 양쪽 테이퍼링: 시작과 끝이 얇고 중간이 두꺼움
            // sin 곡선 사용: T=0,1에서 TaperFactor, T=0.5에서 1.0
            float SinValue = std::sin(T * 3.14159265f);
            Width = BeamWidth * (TaperFactor + (1.0f - TaperFactor) * SinValue);
        }
        OutWidths.Add(Width);
    }
}
