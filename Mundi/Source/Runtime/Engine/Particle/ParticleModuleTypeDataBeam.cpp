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

    // 노이즈가 있으면 알고리즘에 따라 처리, 없으면 직선
    if (NoiseAmplitude > 0.0f)
    {
        if (NoiseAlgorithm == EBeamNoiseAlgorithm::MidpointDisplacement)
        {
            // Midpoint Displacement 방식 (지직거리는 번개 효과)
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
        else // PerlinNoise
        {
            // 연속 Perlin 노이즈 방식 - 부드럽게 출렁이는 효과
            int32 NumPoints = SegmentCount + 1;
            OutPoints.Reserve(NumPoints);

            // 빔 방향 벡터 계산
            FVector BeamDir = (EndPoint - StartPoint).GetNormalized();

            // 변위를 위한 수직 벡터들 계산
            FVector WorldUp = FVector(0.0f, 0.0f, 1.0f);
            FVector Right = FVector::Cross(BeamDir, WorldUp);
            if (Right.Size() < 0.001f)
            {
                // 빔이 수직인 경우 대체 벡터 사용
                Right = FVector::Cross(BeamDir, FVector(0.0f, 1.0f, 0.0f));
            }
            Right = Right.GetNormalized();
            FVector Up = FVector::Cross(Right, BeamDir).GetNormalized();

            for (int32 i = 0; i < NumPoints; ++i)
            {
                float T = static_cast<float>(i) / static_cast<float>(SegmentCount);
                FVector BasePoint = StartPoint + (EndPoint - StartPoint) * T;

                // Perlin 노이즈로 연속적 변위 (시간에 따라 부드럽게 변함)
                float NoiseInputX = T * NoiseFrequency;
                float NoiseInputTime = Time * JitterFrequency * 0.1f;  // JitterFrequency를 속도로 활용

                float NoiseX = FNoiseGenerator::Perlin3D(NoiseInputX, NoiseInputTime, 0.0f);
                float NoiseY = FNoiseGenerator::Perlin3D(NoiseInputX, NoiseInputTime, 100.0f);

                // 양 끝은 변위 감소 (시작점과 끝점은 고정)
                float EdgeFalloff = 4.0f * T * (1.0f - T);  // 0에서 시작, 0.5에서 최대, 1에서 0

                // DisplacementDecay 적용 (끝으로 갈수록 감소)
                float DecayFactor = std::pow(1.0f - T, DisplacementDecay);

                FVector Displacement = (Right * NoiseX + Up * NoiseY) * NoiseAmplitude * EdgeFalloff * DecayFactor;

                OutPoints.Add(BasePoint + Displacement);
            }
        }
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
