#pragma once

#include "ParticleModule.h"
#include "Statistics.h"
#include "Color.h"
#include "UParticleModuleColorOverLife.generated.h"

struct FBaseParticle;

// ============================================================================
// UParticleModuleColorOverLife
// ============================================================================
// 파티클의 수명에 따라 색상을 변화시키는 모듈입니다.
//
// 주요 기능:
// 1. RelativeTime (0.0 ~ 1.0) 기반 색상 보간
// 2. RGB와 Alpha 개별 제어
// 3. Curve 모드로 복잡한 색상 변화 지원
//
// 적용 대상:
// - Sprite: 파티클 수명에 따른 색상 변화
// - Beam: T (0=시작점, 1=끝점) 기반 그라데이션 (렌더링 시 적용)
// - Ribbon: T (0=Head, 1=Tail) 기반 그라데이션 (렌더링 시 적용)
//
// 동작 방식:
// - Update()에서 매 프레임 호출
// - BaseColor * ColorOverLife = 최종 Color
// ============================================================================
UCLASS(DisplayName="수명별 색상", Description="파티클의 수명에 따라 색상을 변화시킵니다.")
class UParticleModuleColorOverLife : public UParticleModule
{
public:
    UParticleModuleColorOverLife();
    ~UParticleModuleColorOverLife() = default;

    GENERATED_REFLECTION_BODY()

    // ========================================================================
    // UParticleModule 인터페이스 구현
    // ========================================================================

    // Spawn: 초기 설정 (필요 시)
    void Spawn(FParticleContext& Context, float EmitterTime) override;

    // Update: 매 프레임 색상 업데이트
    void Update(FParticleContext& Context, float DeltaTime) override;

    // ========================================================================
    // Getters
    // ========================================================================
    const FRawDistribution<FVector>& GetColorOverLife() const { return ColorOverLife; }
    const FRawDistribution<float>& GetAlphaOverLife() const { return AlphaOverLife; }
    bool GetClampAlpha() const { return bClampAlpha; }

    // ========================================================================
    // Setters
    // ========================================================================
    void SetColorOverLife(const FRawDistribution<FVector>& InColor) { ColorOverLife = InColor; }
    void SetAlphaOverLife(const FRawDistribution<float>& InAlpha) { AlphaOverLife = InAlpha; }
    void SetClampAlpha(bool bClamp) { bClampAlpha = bClamp; }

    // ========================================================================
    // 헬퍼 함수 - Curve 모드 설정
    // ========================================================================

    // 색상 커브에 키프레임 추가 (Time: 0.0 ~ 1.0)
    void AddColorKey(float Time, const FVector& Color);

    // 알파 커브에 키프레임 추가 (Time: 0.0 ~ 1.0)
    void AddAlphaKey(float Time, float Alpha);

    // 커브 초기화
    void ClearColorCurve();
    void ClearAlphaCurve();

    // ========================================================================
    // 헬퍼 함수 - 간편 설정
    // ========================================================================

    // 시작~끝 색상 그라데이션 (Uniform 모드 사용)
    void SetColorFade(const FVector& StartColor, const FVector& EndColor);

    // 시작~끝 알파 페이드 (예: 1.0 → 0.0 으로 사라짐)
    void SetAlphaFade(float StartAlpha, float EndAlpha);

    // 색상 페이드아웃 (흰색 → 검정)
    void SetFadeToBlack();

    // 알파 페이드아웃 (1.0 → 0.0)
    void SetFadeOut();

    // T값 (0~1)에 따른 색상 직접 계산 (Beam/Ribbon 렌더링용)
    FLinearColor EvaluateColorAtTime(float T) const;

private:
    // ========================================================================
    // 색상 관련 프로퍼티
    // ========================================================================

    // ------------------------------------------------------------------------
    // ColorOverLife (수명별 색상)
    // ------------------------------------------------------------------------
    // RelativeTime (0.0 ~ 1.0)에 따른 RGB 색상입니다.
    //
    // Curve 모드 사용 시:
    // - 키프레임의 InVal이 RelativeTime
    // - 키프레임의 OutVal이 해당 시점의 색상
    //
    // Uniform 모드 사용 시:
    // - Min = 시작 색상 (T=0)
    // - Max = 끝 색상 (T=1)
    // - RelativeTime으로 선형 보간
    //
    // 예시 (불꽃):
    // - T=0.0: (1, 1, 0.5) 밝은 노랑
    // - T=0.5: (1, 0.5, 0) 주황
    // - T=1.0: (0.5, 0, 0) 어두운 빨강
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[색상]", Tooltip="수명에 따른 RGB 색상 변화")
    FRawDistribution<FVector> ColorOverLife{};

    // ------------------------------------------------------------------------
    // AlphaOverLife (수명별 알파)
    // ------------------------------------------------------------------------
    // RelativeTime (0.0 ~ 1.0)에 따른 알파값입니다.
    //
    // 예시:
    // - T=0.0: 1.0 (불투명)
    // - T=0.5: 0.8 (약간 투명)
    // - T=1.0: 0.0 (완전 투명)
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[색상]", Tooltip="수명에 따른 알파값 변화")
    FRawDistribution<float> AlphaOverLife{};

    // ------------------------------------------------------------------------
    // bClampAlpha (알파 클램핑)
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[색상]", Tooltip="알파값을 0~1 범위로 제한")
    bool bClampAlpha = true;
};
