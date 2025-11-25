#include "pch.h"
#include "ParticleModuleSizeOverLife.h"
#include "ParticleData.h"
#include "ParticleSystemComponent.h"

IMPLEMENT_CLASS(UParticleModuleSizeOverLife)

// ============================================================================
// 생성자
// ============================================================================
UParticleModuleSizeOverLife::UParticleModuleSizeOverLife()
{
    // 기본값: 스케일 유지 (1.0 → 1.0)
    ScaleOverLife.Min = FVector(1.0f, 1.0f, 1.0f);
    ScaleOverLife.Max = FVector(1.0f, 1.0f, 1.0f);

    bUseUniformScale = true;
}

// ============================================================================
// Spawn 함수
// ============================================================================
void UParticleModuleSizeOverLife::Spawn(FParticleContext& Context, float EmitterTime)
{
    // SizeOverLife는 Update에서 처리
    // Spawn 시점에서는 초기 크기가 이미 ParticleModuleSize에서 설정됨
}

// ============================================================================
// Update 함수
// ============================================================================
// 매 프레임 호출되어 RelativeTime에 따라 크기를 업데이트합니다.
//
// 처리 순서:
// 1. RelativeTime 가져오기 (0.0 ~ 1.0)
// 2. ScaleOverLife에서 스케일 계산
// 3. BaseSize와 곱하여 최종 Size 설정
// ============================================================================
void UParticleModuleSizeOverLife::Update(FParticleContext& Context, float DeltaTime)
{
    FBaseParticle* Particle = Context.Particle;
    if (!Particle) return;

    // ------------------------------------------------------------------------
    // Step 1: RelativeTime 가져오기
    // ------------------------------------------------------------------------
    float T = Particle->RelativeTime;
    T = FMath::Clamp(T, 0.0f, 1.0f);

    // ------------------------------------------------------------------------
    // Step 2: 스케일 계산
    // ------------------------------------------------------------------------
    FVector Scale;
    if (ScaleOverLife.Mode == EDistributionMode::Curve && ScaleOverLife.Curve.HasKeys())
    {
        // Curve 모드: 키프레임 기반 보간
        Scale = ScaleOverLife.Curve.Eval(T);
    }
    else
    {
        // Uniform 모드: Min~Max 선형 보간
        // T=0 → Min, T=1 → Max
        Scale = FMath::Lerp(ScaleOverLife.Min, ScaleOverLife.Max, T);
    }

    // 균일 스케일 처리
    if (bUseUniformScale)
    {
        Scale.Y = Scale.X;
        Scale.Z = Scale.X;
    }

    // ------------------------------------------------------------------------
    // Step 3: BaseSize와 곱하여 최종 Size 설정
    // ------------------------------------------------------------------------
    // BaseSize는 ParticleModuleSize에서 설정된 초기 크기
    // ScaleOverLife 값을 곱하여 수명에 따른 변화 적용
    //
    // 예시:
    // - BaseSize = (2, 2, 2)
    // - ScaleOverLife = (0.5, 0.5, 0.5) at T=0
    // - 결과: Size = (1, 1, 1)
    // ------------------------------------------------------------------------
    Particle->Size.X = Particle->BaseSize.X * Scale.X;
    Particle->Size.Y = Particle->BaseSize.Y * Scale.Y;
    Particle->Size.Z = Particle->BaseSize.Z * Scale.Z;
}

// ============================================================================
// 헬퍼 함수 - Curve 모드 설정
// ============================================================================
void UParticleModuleSizeOverLife::AddScaleKey(float Time, const FVector& Scale)
{
    ScaleOverLife.Mode = EDistributionMode::Curve;
    ScaleOverLife.Curve.AddPoint(Time, Scale);
}

void UParticleModuleSizeOverLife::AddUniformScaleKey(float Time, float Scale)
{
    AddScaleKey(Time, FVector(Scale, Scale, Scale));
}

void UParticleModuleSizeOverLife::ClearScaleCurve()
{
    ScaleOverLife.Curve.Reset();
    ScaleOverLife.Mode = EDistributionMode::Uniform;
}

// ============================================================================
// 헬퍼 함수 - 간편 설정
// ============================================================================
void UParticleModuleSizeOverLife::SetScaleFade(const FVector& StartScale, const FVector& EndScale)
{
    ScaleOverLife.Mode = EDistributionMode::Uniform;
    ScaleOverLife.Min = StartScale;
    ScaleOverLife.Max = EndScale;
}

void UParticleModuleSizeOverLife::SetUniformScaleFade(float StartScale, float EndScale)
{
    bUseUniformScale = true;
    SetScaleFade(
        FVector(StartScale, StartScale, StartScale),
        FVector(EndScale, EndScale, EndScale)
    );
}

void UParticleModuleSizeOverLife::SetGrow(float StartScale, float EndScale)
{
    SetUniformScaleFade(StartScale, EndScale);
}

void UParticleModuleSizeOverLife::SetShrink(float StartScale, float EndScale)
{
    SetUniformScaleFade(StartScale, EndScale);
}

void UParticleModuleSizeOverLife::SetPulse(float MinScale, float MaxScale)
{
    // Curve 모드로 팽창 후 수축
    ClearScaleCurve();
    bUseUniformScale = true;

    AddUniformScaleKey(0.0f, MinScale);   // 시작: 작게
    AddUniformScaleKey(0.5f, MaxScale);   // 중간: 크게
    AddUniformScaleKey(1.0f, MinScale);   // 끝: 작게
}

// ============================================================================
// EvaluateScaleAtTime
// ============================================================================
// T값 (0~1)에 따른 스케일을 직접 계산합니다.
// Beam/Ribbon 렌더링에서 세그먼트별 폭 계산에 사용됩니다.
//
// 사용 예시 (Ribbon):
// for (int i = 0; i < ParticleCount; ++i)
// {
//     float T = (float)i / (ParticleCount - 1);  // 0=Head, 1=Tail
//     FVector Scale = SizeOverLifeModule->EvaluateScaleAtTime(T);
//     float Width = BaseWidth * Scale.X;
// }
// ============================================================================
FVector UParticleModuleSizeOverLife::EvaluateScaleAtTime(float T) const
{
    T = FMath::Clamp(T, 0.0f, 1.0f);

    FVector Scale;
    if (ScaleOverLife.Mode == EDistributionMode::Curve && ScaleOverLife.Curve.HasKeys())
    {
        Scale = const_cast<FInterpCurve<FVector>&>(ScaleOverLife.Curve).Eval(T);
    }
    else
    {
        Scale = FMath::Lerp(ScaleOverLife.Min, ScaleOverLife.Max, T);
    }

    if (bUseUniformScale)
    {
        Scale.Y = Scale.X;
        Scale.Z = Scale.X;
    }

    return Scale;
}

float UParticleModuleSizeOverLife::EvaluateUniformScaleAtTime(float T) const
{
    return EvaluateScaleAtTime(T).X;
}
