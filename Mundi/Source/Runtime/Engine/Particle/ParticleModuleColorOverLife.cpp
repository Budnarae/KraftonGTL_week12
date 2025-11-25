#include "pch.h"
#include "ParticleModuleColorOverLife.h"
#include "ParticleData.h"
#include "ParticleSystemComponent.h"

IMPLEMENT_CLASS(UParticleModuleColorOverLife)

// ============================================================================
// 생성자
// ============================================================================
UParticleModuleColorOverLife::UParticleModuleColorOverLife()
{
    // 기본값: 색상 유지 (흰색 → 흰색)
    ColorOverLife.Min = FVector(1.0f, 1.0f, 1.0f);
    ColorOverLife.Max = FVector(1.0f, 1.0f, 1.0f);

    // 기본값: 알파 페이드아웃 (1.0 → 0.0)
    AlphaOverLife.Min = 1.0f;
    AlphaOverLife.Max = 0.0f;

    bClampAlpha = true;
}

// ============================================================================
// Spawn 함수
// ============================================================================
void UParticleModuleColorOverLife::Spawn(FParticleContext& Context, float EmitterTime)
{
    // ColorOverLife는 Update에서 처리
    // Spawn 시점에서는 초기 색상이 이미 ParticleModuleColor에서 설정됨
}

// ============================================================================
// Update 함수
// ============================================================================
// 매 프레임 호출되어 RelativeTime에 따라 색상을 업데이트합니다.
//
// 처리 순서:
// 1. RelativeTime 가져오기 (0.0 ~ 1.0)
// 2. ColorOverLife에서 색상 계산
// 3. AlphaOverLife에서 알파 계산
// 4. BaseColor와 곱하여 최종 Color 설정
// ============================================================================
void UParticleModuleColorOverLife::Update(FParticleContext& Context, float DeltaTime)
{
    FBaseParticle* Particle = Context.Particle;
    if (!Particle) return;

    // ------------------------------------------------------------------------
    // Step 1: RelativeTime 가져오기
    // ------------------------------------------------------------------------
    // RelativeTime: 파티클 수명 진행도
    // - 0.0: 막 스폰됨
    // - 0.5: 수명의 절반
    // - 1.0: 수명 끝
    // ------------------------------------------------------------------------
    float T = Particle->RelativeTime;
    T = FMath::Clamp(T, 0.0f, 1.0f);

    // ------------------------------------------------------------------------
    // Step 2: 색상 계산
    // ------------------------------------------------------------------------
    FVector ColorVec;
    if (ColorOverLife.Mode == EDistributionMode::Curve && ColorOverLife.Curve.HasKeys())
    {
        // Curve 모드: 키프레임 기반 보간
        ColorVec = ColorOverLife.Curve.Eval(T);
    }
    else
    {
        // Uniform 모드: Min~Max 선형 보간
        // T=0 → Min, T=1 → Max
        ColorVec = FMath::Lerp(ColorOverLife.Min, ColorOverLife.Max, T);
    }

    // ------------------------------------------------------------------------
    // Step 3: 알파 계산
    // ------------------------------------------------------------------------
    float Alpha;
    if (AlphaOverLife.Mode == EDistributionMode::Curve && AlphaOverLife.Curve.HasKeys())
    {
        Alpha = AlphaOverLife.Curve.Eval(T);
    }
    else
    {
        // Uniform 모드: Min~Max 선형 보간
        Alpha = FMath::Lerp(AlphaOverLife.Min, AlphaOverLife.Max, T);
    }

    if (bClampAlpha)
    {
        Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
    }

    // ------------------------------------------------------------------------
    // Step 4: BaseColor와 곱하여 최종 Color 설정
    // ------------------------------------------------------------------------
    // BaseColor는 ParticleModuleColor에서 설정된 초기 색상
    // ColorOverLife 값을 곱하여 수명에 따른 변화 적용
    //
    // 예시:
    // - BaseColor = (1, 0, 0) 빨강
    // - ColorOverLife = (1, 1, 1) → (0.5, 0.5, 0.5)
    // - 결과: 빨강 → 어두운 빨강
    // ------------------------------------------------------------------------
    Particle->Color.R = Particle->BaseColor.R * ColorVec.X;
    Particle->Color.G = Particle->BaseColor.G * ColorVec.Y;
    Particle->Color.B = Particle->BaseColor.B * ColorVec.Z;
    Particle->Color.A = Particle->BaseColor.A * Alpha;
}

// ============================================================================
// 헬퍼 함수 - Curve 모드 설정
// ============================================================================
void UParticleModuleColorOverLife::AddColorKey(float Time, const FVector& Color)
{
    ColorOverLife.Mode = EDistributionMode::Curve;
    ColorOverLife.Curve.AddPoint(Time, Color);
}

void UParticleModuleColorOverLife::AddAlphaKey(float Time, float Alpha)
{
    AlphaOverLife.Mode = EDistributionMode::Curve;
    AlphaOverLife.Curve.AddPoint(Time, Alpha);
}

void UParticleModuleColorOverLife::ClearColorCurve()
{
    ColorOverLife.Curve.Reset();
    ColorOverLife.Mode = EDistributionMode::Uniform;
}

void UParticleModuleColorOverLife::ClearAlphaCurve()
{
    AlphaOverLife.Curve.Reset();
    AlphaOverLife.Mode = EDistributionMode::Uniform;
}

// ============================================================================
// 헬퍼 함수 - 간편 설정
// ============================================================================
void UParticleModuleColorOverLife::SetColorFade(const FVector& StartColor, const FVector& EndColor)
{
    ColorOverLife.Mode = EDistributionMode::Uniform;
    ColorOverLife.Min = StartColor;
    ColorOverLife.Max = EndColor;
}

void UParticleModuleColorOverLife::SetAlphaFade(float StartAlpha, float EndAlpha)
{
    AlphaOverLife.Mode = EDistributionMode::Uniform;
    AlphaOverLife.Min = StartAlpha;
    AlphaOverLife.Max = EndAlpha;
}

void UParticleModuleColorOverLife::SetFadeToBlack()
{
    SetColorFade(FVector(1.0f, 1.0f, 1.0f), FVector(0.0f, 0.0f, 0.0f));
}

void UParticleModuleColorOverLife::SetFadeOut()
{
    SetAlphaFade(1.0f, 0.0f);
}

// ============================================================================
// EvaluateColorAtTime
// ============================================================================
// T값 (0~1)에 따른 색상을 직접 계산합니다.
// Beam/Ribbon 렌더링에서 세그먼트별 색상 계산에 사용됩니다.
//
// 사용 예시 (Beam):
// for (int i = 0; i < SegmentCount; ++i)
// {
//     float T = (float)i / (SegmentCount - 1);
//     FLinearColor SegmentColor = ColorOverLifeModule->EvaluateColorAtTime(T);
// }
// ============================================================================
FLinearColor UParticleModuleColorOverLife::EvaluateColorAtTime(float T) const
{
    T = FMath::Clamp(T, 0.0f, 1.0f);

    // 색상 계산
    FVector ColorVec;
    if (ColorOverLife.Mode == EDistributionMode::Curve && ColorOverLife.Curve.HasKeys())
    {
        ColorVec = const_cast<FInterpCurve<FVector>&>(ColorOverLife.Curve).Eval(T);
    }
    else
    {
        ColorVec = FMath::Lerp(ColorOverLife.Min, ColorOverLife.Max, T);
    }

    // 알파 계산
    float Alpha;
    if (AlphaOverLife.Mode == EDistributionMode::Curve && AlphaOverLife.Curve.HasKeys())
    {
        Alpha = const_cast<FInterpCurve<float>&>(AlphaOverLife.Curve).Eval(T);
    }
    else
    {
        Alpha = FMath::Lerp(AlphaOverLife.Min, AlphaOverLife.Max, T);
    }

    if (bClampAlpha)
    {
        Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
    }

    return FLinearColor(ColorVec.X, ColorVec.Y, ColorVec.Z, Alpha);
}
