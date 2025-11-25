#include "pch.h"
#include "ParticleModuleNoise.h"
#include "ParticleData.h"
#include "ParticleSystemComponent.h"
#include "Source/Runtime/Engine/PCG/NoiseGenerator.h"
#include <cmath>

IMPLEMENT_CLASS(UParticleModuleNoise)

// ============================================================================
// 생성자
// ============================================================================
UParticleModuleNoise::UParticleModuleNoise()
{
    NoiseAlgorithm = ENoiseAlgorithm::Perlin;
    NoiseAmplitude = 1.0f;
    NoiseFrequency = 1.0f;
    NoiseSpeed = 1.0f;
    JitterFrequency = 20.0f;
    DisplacementDecay = 0.5f;
    bApplyToLocation = true;
    bScaleByLife = false;
    AccumulatedTime = 0.0f;
}

// ============================================================================
// Spawn 함수
// ============================================================================
void UParticleModuleNoise::Spawn(FParticleContext& Context, float EmitterTime)
{
    // 노이즈는 Update에서 처리
    // Spawn 시점에서는 특별한 처리 없음
}

// ============================================================================
// Update 함수
// ============================================================================
// 매 프레임 호출되어 파티클 위치에 노이즈를 적용합니다.
// ============================================================================
void UParticleModuleNoise::Update(FParticleContext& Context, float DeltaTime)
{
    FBaseParticle* Particle = Context.Particle;
    if (!Particle) return;

    // 시간 누적
    AccumulatedTime += DeltaTime;

    // bApplyToLocation이 false면 위치에 적용하지 않음
    // (Beam/Ribbon에서 직접 CalculateNoiseOffset() 호출)
    if (!bApplyToLocation) return;

    // ------------------------------------------------------------------------
    // 노이즈 변위 계산을 위한 기준 벡터 설정
    // ------------------------------------------------------------------------
    // 파티클의 속도 방향을 기준으로 수직 벡터 계산
    FVector Forward = Particle->Velocity;
    if (Forward.Size() < 0.001f)
    {
        Forward = FVector(0.0f, 0.0f, 1.0f);  // 속도 없으면 Z축 기준
    }
    Forward = Forward.GetNormalized();

    FVector WorldUp = FVector(0.0f, 0.0f, 1.0f);
    FVector Right = FVector::Cross(Forward, WorldUp);
    if (Right.Size() < 0.001f)
    {
        Right = FVector::Cross(Forward, FVector(0.0f, 1.0f, 0.0f));
    }
    Right = Right.GetNormalized();
    FVector Up = FVector::Cross(Right, Forward).GetNormalized();

    // ------------------------------------------------------------------------
    // 노이즈 오프셋 계산
    // ------------------------------------------------------------------------
    // 파티클의 RelativeTime을 T로 사용
    float T = Particle->RelativeTime;
    FVector NoiseOffset = CalculateNoiseOffset(T, AccumulatedTime, Right, Up);

    // ------------------------------------------------------------------------
    // 수명에 따른 스케일 적용
    // ------------------------------------------------------------------------
    if (bScaleByLife)
    {
        // 파티클이 오래될수록 노이즈 감소
        float LifeScale = 1.0f - Particle->RelativeTime;
        NoiseOffset *= LifeScale;
    }

    // ------------------------------------------------------------------------
    // 위치에 노이즈 적용
    // ------------------------------------------------------------------------
    Particle->Location += NoiseOffset;
}

// ============================================================================
// CalculateNoiseOffset
// ============================================================================
// T값과 시간에 따른 노이즈 오프셋을 계산합니다.
// Beam/Ribbon 렌더링에서 세그먼트별 변위 계산에 사용됩니다.
//
// @param T: 위치 비율 (0~1)
// @param Time: 현재 시간
// @param Right: 수평 변위 기준 벡터
// @param Up: 수직 변위 기준 벡터
// @return 월드 공간 변위 벡터
// ============================================================================
FVector UParticleModuleNoise::CalculateNoiseOffset(
    float T,
    float Time,
    const FVector& Right,
    const FVector& Up
) const
{
    if (NoiseAmplitude <= 0.0f) return FVector::Zero();

    FVector Offset = FVector::Zero();

    if (NoiseAlgorithm == ENoiseAlgorithm::Perlin)
    {
        // ------------------------------------------------------------------------
        // Perlin 노이즈 - 부드러운 연속 변위
        // ------------------------------------------------------------------------
        // 공간 좌표: T * NoiseFrequency
        // 시간 좌표: Time * NoiseSpeed
        // 두 개의 독립적인 노이즈 값으로 Right/Up 방향 변위 계산
        // ------------------------------------------------------------------------
        float NoiseInputX = T * NoiseFrequency;
        float NoiseInputTime = Time * NoiseSpeed;

        // 두 개의 다른 시드로 X/Y 방향 노이즈 생성
        float NoiseX = FNoiseGenerator::Perlin3D(NoiseInputX, NoiseInputTime, 0.0f);
        float NoiseY = FNoiseGenerator::Perlin3D(NoiseInputX, NoiseInputTime, 100.0f);

        // 양 끝은 변위 감소 (시작점과 끝점은 고정)
        // 0에서 시작, 0.5에서 최대(1.0), 1에서 0
        float EdgeFalloff = 4.0f * T * (1.0f - T);

        Offset = (Right * NoiseX + Up * NoiseY) * NoiseAmplitude * EdgeFalloff;
    }
    else // MidpointDisplacement
    {
        // ------------------------------------------------------------------------
        // Midpoint Displacement - 불규칙한 변위 (번개 효과)
        // ------------------------------------------------------------------------
        // 시간을 양자화하여 지직거리는 효과 생성
        // JitterFrequency가 높을수록 빠르게 변화
        // ------------------------------------------------------------------------
        int32 TimeSeed = static_cast<int32>(Time * JitterFrequency);

        // T 위치에서의 변위 계산
        // 랜덤 시드: TimeSeed + T 기반
        uint32 Seed = static_cast<uint32>(TimeSeed * 1000 + static_cast<int32>(T * 1000));

        // 간단한 해시 함수로 의사 랜덤 생성
        Seed = (Seed * 1103515245 + 12345) & 0x7FFFFFFF;
        float RandX = (static_cast<float>(Seed) / 0x7FFFFFFF) * 2.0f - 1.0f;

        Seed = (Seed * 1103515245 + 12345) & 0x7FFFFFFF;
        float RandY = (static_cast<float>(Seed) / 0x7FFFFFFF) * 2.0f - 1.0f;

        // 양 끝 감쇠 + Decay 적용
        float EdgeFalloff = 4.0f * T * (1.0f - T);
        float DecayFactor = std::pow(1.0f - T, DisplacementDecay);

        Offset = (Right * RandX + Up * RandY) * NoiseAmplitude * EdgeFalloff * DecayFactor;
    }

    return Offset;
}

// ============================================================================
// GenerateLightningPoints
// ============================================================================
// Midpoint Displacement 알고리즘으로 번개 포인트를 생성합니다.
// FNoiseGenerator의 GenerateLightningPoints를 래핑합니다.
//
// @param StartPoint: 시작점
// @param EndPoint: 끝점
// @param Depth: 재귀 깊이 (세그먼트 수 = 2^Depth)
// @param Time: 현재 시간 (시드 계산용)
// @param OutPoints: 출력 포인트 배열
// ============================================================================
void UParticleModuleNoise::GenerateLightningPoints(
    const FVector& StartPoint,
    const FVector& EndPoint,
    int32 Depth,
    float Time,
    TArray<FVector>& OutPoints
) const
{
    // 시간 기반 시드 계산
    int32 TimeSeed = static_cast<int32>(Time * JitterFrequency);

    // FNoiseGenerator의 함수 호출
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

// ============================================================================
// 헬퍼 함수 - 프리셋
// ============================================================================
void UParticleModuleNoise::SetupSmoothNoise(float Amplitude, float Frequency, float Speed)
{
    NoiseAlgorithm = ENoiseAlgorithm::Perlin;
    NoiseAmplitude = Amplitude;
    NoiseFrequency = Frequency;
    NoiseSpeed = Speed;
    bApplyToLocation = true;
    bScaleByLife = false;
}

void UParticleModuleNoise::SetupLightningNoise(float Amplitude, float JitterFreq)
{
    NoiseAlgorithm = ENoiseAlgorithm::MidpointDisplacement;
    NoiseAmplitude = Amplitude;
    JitterFrequency = JitterFreq;
    DisplacementDecay = 0.5f;
    bApplyToLocation = false;  // Beam에서 직접 처리
    bScaleByLife = false;
}

void UParticleModuleNoise::SetupVibrationNoise(float Amplitude, float Frequency)
{
    NoiseAlgorithm = ENoiseAlgorithm::Perlin;
    NoiseAmplitude = Amplitude;
    NoiseFrequency = Frequency;
    NoiseSpeed = 5.0f;  // 빠른 변화
    bApplyToLocation = true;
    bScaleByLife = true;  // 시간이 지나면서 감소
}
