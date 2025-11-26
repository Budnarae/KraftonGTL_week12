#include "pch.h"
#include "ParticleModuleTypeDataBeam.h"
#include "ParticleModuleBeamNoise.h"  // FBeamNoiseParams
#include "ParticleModuleBeamWidth.h"  // FBeamWidthParams
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
    const FMatrix& EmitterRotation,
    TArray<FVector>& OutPoints,
    TArray<float>& OutWidths,
    float Time,
    const FBeamNoiseParams* NoiseParams,
    const FBeamWidthParams* WidthParams
) const
{
    OutPoints.Empty();
    OutWidths.Empty();

    if (SegmentCount <= 0)
        return;

    // 로컬 공간 값을 월드 공간으로 변환
    FVector WorldSourcePoint = SourcePoint * EmitterRotation;
    FVector WorldBeamDirection = BeamDirection * EmitterRotation;

    // Calculate start and end points based on beam method
    FVector StartPoint = EmitterLocation + WorldSourcePoint;
    FVector EndPoint;

    switch (BeamMethod)
    {
    case EBeamMethod::Distance:
        EndPoint = StartPoint + WorldBeamDirection.GetNormalized() * BeamLength;
        break;
    case EBeamMethod::Target:
        // TargetPoint도 로컬 공간으로 정의된 경우 변환 필요
        EndPoint = EmitterLocation + (TargetPoint * EmitterRotation);
        break;
    case EBeamMethod::Source:
    default:
        {
            FVector DefaultDir = FVector(BeamLength, 0.0f, 0.0f) * EmitterRotation;
            EndPoint = StartPoint + DefaultDir;
        }
        break;
    }

    // 노이즈 파라미터가 전달되었고 Amplitude > 0이면 노이즈 적용
    if (NoiseParams && NoiseParams->Amplitude > 0.0f)
    {
        if (NoiseParams->Algorithm == EBeamNoiseAlgorithm::MidpointDisplacement)
        {
            // Midpoint Displacement 방식 (지직거리는 번개 효과)
            int32 Depth = 1;
            int32 Temp = SegmentCount;
            while (Temp > 1)
            {
                Temp >>= 1;
                Depth++;
            }
            Depth = std::min(Depth, 6);  // 최대 depth 6 (64 세그먼트)

            // 시간을 양자화하여 지직거리는 효과
            int32 TimeSeed = static_cast<int32>(Time * NoiseParams->JitterFrequency);

            // Midpoint Displacement로 번개 포인트 생성
            FNoiseGenerator::GenerateLightningPoints(
                StartPoint,
                EndPoint,
                Depth,
                NoiseParams->Amplitude,
                TimeSeed,
                OutPoints,
                NoiseParams->DisplacementDecay
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

                // Perlin 노이즈로 연속적 변위
                float NoiseInputX = T * NoiseParams->Frequency;
                float NoiseInputTime = Time * NoiseParams->JitterFrequency * 0.1f;

                float NoiseX = FNoiseGenerator::Perlin3D(NoiseInputX, NoiseInputTime, 0.0f);
                float NoiseY = FNoiseGenerator::Perlin3D(NoiseInputX, NoiseInputTime, 100.0f);

                // 양 끝은 변위 감소
                float EdgeFalloff = 4.0f * T * (1.0f - T);

                // DisplacementDecay 적용
                float DecayFactor = std::pow(1.0f - T, NoiseParams->DisplacementDecay);

                FVector Displacement = (Right * NoiseX + Up * NoiseY) * NoiseParams->Amplitude * EdgeFalloff * DecayFactor;

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

    // Width 계산 - BeamWidth를 기본 너비로 사용, WidthParams는 스케일 팩터
    constexpr float MinWidth = 0.01f;  // 최소 너비 (0 방지)

    OutWidths.Reserve(OutPoints.Num());
    for (int32 i = 0; i < OutPoints.Num(); ++i)
    {
        float T = static_cast<float>(i) / static_cast<float>(OutPoints.Num() - 1);

        float Width = BeamWidth;  // TypeDataBeam의 기본 너비

        if (WidthParams)
        {
            // BeamWidth 모듈이 있으면 스케일 팩터로 적용
            float Scale = 1.0f;
            int32 Method = WidthParams->TaperMethodInt;

            switch (Method)
            {
            case 0:  // None
                // 테이퍼 없음 - StartWidthScale 사용
                Scale = WidthParams->StartWidthScale;
                break;

            case 1:  // Linear
                // 선형 보간: Start -> End
                Scale = WidthParams->StartWidthScale + (WidthParams->EndWidthScale - WidthParams->StartWidthScale) * T;
                break;

            case 2:  // Sine (번개 효과 - 양쪽 가늘고 중간 두꺼움)
            default:
                // 사인 곡선: 양쪽 끝이 가늘고 중간이 두꺼움
                // sin(0) = 0, sin(π/2) = 1, sin(π) = 0
                {
                    float SinValue = std::sin(T * 3.14159265f);
                    // 양 끝에서는 각각의 스케일, 중간에서는 1.0 (풀 스케일)
                    float EdgeScale = WidthParams->StartWidthScale + (WidthParams->EndWidthScale - WidthParams->StartWidthScale) * T;
                    // 중간에서 EdgeScale보다 1.0에 가깝게 부풀림
                    Scale = EdgeScale + (1.0f - EdgeScale) * SinValue;
                }
                break;

            case 3:  // EaseOut
                // Ease Out: 시작에서 두껍고 끝으로 갈수록 급격히 얇아짐
                {
                    float EaseT = 1.0f - (1.0f - T) * (1.0f - T);  // quadratic ease out
                    Scale = WidthParams->StartWidthScale + (WidthParams->EndWidthScale - WidthParams->StartWidthScale) * EaseT;
                }
                break;
            }

            Width = BeamWidth * Scale;
        }
        else if (TaperMethod != EBeamTaperMethod::None)
        {
            // 모듈 없으면 기존 TypeData의 테이퍼 설정 사용
            float Scale = 1.0f;
            switch (TaperMethod)
            {
            case EBeamTaperMethod::Source:
                // 소스에서 가늘어짐: T=0에서 TaperFactor, T=1에서 1.0
                Scale = TaperFactor + (1.0f - TaperFactor) * T;
                break;
            case EBeamTaperMethod::Target:
                // 타겟에서 가늘어짐: T=0에서 1.0, T=1에서 TaperFactor
                Scale = 1.0f - (1.0f - TaperFactor) * T;
                break;
            case EBeamTaperMethod::Both:
                // 양쪽 테이퍼링: 시작과 끝이 얇고 중간이 두꺼움
                {
                    float SinValue = std::sin(T * 3.14159265f);
                    Scale = TaperFactor + (1.0f - TaperFactor) * SinValue;
                }
                break;
            default:
                break;
            }
            Width = BeamWidth * Scale;
        }

        // 최소 너비 클램핑 (0 너비 방지)
        Width = std::max(Width, MinWidth);

        OutWidths.Add(Width);
    }
}
