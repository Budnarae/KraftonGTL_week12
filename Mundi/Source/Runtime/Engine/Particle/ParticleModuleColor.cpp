#include "pch.h"
#include "ParticleModuleColor.h"

#include "ParticleData.h"
#include "ParticleSystemComponent.h"

// ============================================================================
// 생성자
// ============================================================================
UParticleModuleColor::UParticleModuleColor()
{
    // 기본 색상: 흰색 (1, 1, 1)
    StartColor.Min = FVector(1.0f, 1.0f, 1.0f);
    StartColor.Max = FVector(1.0f, 1.0f, 1.0f);

    // 기본 알파: 1.0 (불투명)
    StartAlpha.Min = 1.0f;
    StartAlpha.Max = 1.0f;

    // 알파 클램핑 기본 활성화
    bClampAlpha = true;
}

// ============================================================================
// Getters
// ============================================================================
const FRawDistribution<FVector>& UParticleModuleColor::GetStartColor() const { return StartColor; }
const FRawDistribution<float>& UParticleModuleColor::GetStartAlpha() const { return StartAlpha; }
bool UParticleModuleColor::GetClampAlpha() const { return bClampAlpha; }

// ============================================================================
// Setters
// ============================================================================
void UParticleModuleColor::SetStartColor(const FRawDistribution<FVector>& InColor) { StartColor = InColor; }
void UParticleModuleColor::SetStartColorMin(const FVector& InMin) { StartColor.Min = InMin; }
void UParticleModuleColor::SetStartColorMax(const FVector& InMax) { StartColor.Max = InMax; }
void UParticleModuleColor::SetStartAlpha(const FRawDistribution<float>& InAlpha) { StartAlpha = InAlpha; }
void UParticleModuleColor::SetStartAlphaMin(float InMin) { StartAlpha.Min = InMin; }
void UParticleModuleColor::SetStartAlphaMax(float InMax) { StartAlpha.Max = InMax; }
void UParticleModuleColor::SetClampAlpha(bool bClamp) { bClampAlpha = bClamp; }

// ============================================================================
// 헬퍼 함수
// ============================================================================
void UParticleModuleColor::SetColorRange(const FVector& InMin, const FVector& InMax)
{
    StartColor.Min = InMin;
    StartColor.Max = InMax;
}

void UParticleModuleColor::SetFixedColor(const FVector& InColor, float InAlpha)
{
    StartColor.Min = InColor;
    StartColor.Max = InColor;
    StartAlpha.Min = InAlpha;
    StartAlpha.Max = InAlpha;
}

void UParticleModuleColor::SetFixedColor(const FLinearColor& InColor)
{
    StartColor.Min = FVector(InColor.R, InColor.G, InColor.B);
    StartColor.Max = FVector(InColor.R, InColor.G, InColor.B);
    StartAlpha.Min = InColor.A;
    StartAlpha.Max = InColor.A;
}

void UParticleModuleColor::SetAlphaRange(float InMin, float InMax)
{
    StartAlpha.Min = InMin;
    StartAlpha.Max = InMax;
}

// ============================================================================
// Spawn 함수
// ============================================================================
// 파티클이 생성될 때 호출되어 색상을 설정합니다.
//
// 처리 순서:
// 1. Distribution에서 RGB 색상 가져오기
// 2. Distribution에서 알파 가져오기
// 3. 알파 클램핑 (옵션)
// 4. FLinearColor로 변환하여 파티클에 적용
// ============================================================================
void UParticleModuleColor::Spawn(FParticleContext& Context, float EmitterTime)
{
    // ------------------------------------------------------------------------
    // Step 0: 파티클 유효성 검사
    // ------------------------------------------------------------------------
    FBaseParticle* Particle = Context.Particle;
    if (!Particle) return;

    // ------------------------------------------------------------------------
    // Step 1: Distribution에서 RGB 색상 가져오기
    // ------------------------------------------------------------------------
    // GetValue는 Distribution의 Mode에 따라 다르게 동작:
    // - Uniform 모드: Min~Max 사이 랜덤 색상 반환
    // - Curve 모드: EmitterTime에 따른 보간 색상 반환
    //
    // FVector로 반환됨:
    // - X = Red
    // - Y = Green
    // - Z = Blue
    //
    // 값 범위:
    // - 일반: 0.0 ~ 1.0
    // - HDR: 1.0 초과 가능 (발광 효과 등)
    // ------------------------------------------------------------------------
    FVector ColorVec = StartColor.GetValue(EmitterTime);

    // ------------------------------------------------------------------------
    // Step 2: Distribution에서 알파 가져오기
    // ------------------------------------------------------------------------
    // 알파는 RGB와 별도의 Distribution으로 처리
    //
    // 이유:
    // - RGB는 색상 팔레트, 알파는 투명도로 역할이 다름
    // - 알파만 시간에 따라 변화시키는 경우가 많음
    // - 클램핑 로직이 필요한 경우가 많음
    // ------------------------------------------------------------------------
    float Alpha = StartAlpha.GetValue(EmitterTime);

    // ------------------------------------------------------------------------
    // Step 3: 알파 클램핑
    // ------------------------------------------------------------------------
    // bClampAlpha가 true면 0~1 범위로 제한
    //
    // 클램핑이 필요한 이유:
    // - 블렌딩 계산에서 예측 가능한 결과
    // - 셰이더 호환성
    //
    // 클램핑을 끄는 경우:
    // - HDR 렌더링
    // - 커스텀 블렌딩 모드
    // ------------------------------------------------------------------------
    if (bClampAlpha)
    {
        Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
    }

    // ------------------------------------------------------------------------
    // Step 4: FLinearColor로 변환하여 파티클에 적용
    // ------------------------------------------------------------------------
    // FLinearColor 구조:
    // - R, G, B: 색상 채널
    // - A: 알파 채널
    //
    // Color: 현재 프레임의 색상 (Update 모듈에서 변경 가능)
    // BaseColor: 초기 색상 (참조용으로 보존)
    //
    // 언리얼에서는 Particle.Color를 직접 할당하지만,
    // 여러 Color 모듈을 스택하려면 곱셈 연산도 고려 가능
    // 현재는 덮어쓰기 방식으로 구현
    // ------------------------------------------------------------------------
    FLinearColor FinalColor(ColorVec.X, ColorVec.Y, ColorVec.Z, Alpha);

    Particle->Color = FinalColor;
    Particle->BaseColor = FinalColor;

    // ------------------------------------------------------------------------
    // 디버그 정보 (필요시 활성화)
    // ------------------------------------------------------------------------
    // UE_LOG("Color Module: Particle color set to (%.2f, %.2f, %.2f, %.2f)",
    //        FinalColor.R, FinalColor.G, FinalColor.B, FinalColor.A);
}
