#pragma once

#include "UParticleModuleColor.generated.h"
#include "Statistics.h"

struct FBaseParticle;

// ============================================================================
// UParticleModuleColor
// ============================================================================
// 파티클의 초기 색상을 설정하는 모듈입니다.
//
// 언리얼 엔진의 UParticleModuleColor를 참고하여 구현되었습니다.
//
// 주요 기능:
// 1. StartColor로 RGB 색상 설정 (FVector: X=R, Y=G, Z=B)
// 2. StartAlpha로 알파값 별도 설정 (0.0 ~ 1.0)
// 3. EmitterTime 기반 시간에 따른 색상 변화 가능
//
// 색상과 알파를 분리하는 이유:
// - RGB는 3D 벡터로 처리하기 편함
// - 알파는 보통 0~1 범위로 클램핑이 필요
// - 각각 독립적인 커브/랜덤 설정 가능
//
// 렌더링 파이프라인에서의 사용:
// - Billboard 셰이더에서 Particle.Color를 읽어 텍스처에 곱함
// - 투명도, 발광 효과 등에 활용
// ============================================================================
UCLASS(DisplayName="파티클 모듈 색상", Description="파티클의 초기 색상을 지정합니다.")
class UParticleModuleColor : public UParticleModule
{
public:
    UParticleModuleColor();
    ~UParticleModuleColor() = default;

    GENERATED_REFLECTION_BODY()

    // ========================================================================
    // UParticleModule 인터페이스 구현
    // ========================================================================

    // Spawn: 파티클이 생성될 때 호출됨
    // - Color와 BaseColor를 설정
    void Spawn(FParticleContext& Context, float EmitterTime) override;

    // ========================================================================
    // Getters
    // ========================================================================
    const FRawDistribution<FVector>& GetStartColor() const;
    const FRawDistribution<float>& GetStartAlpha() const;
    bool GetClampAlpha() const;

    // ========================================================================
    // Setters
    // ========================================================================
    void SetStartColor(const FRawDistribution<FVector>& InColor);
    void SetStartColorMin(const FVector& InMin);
    void SetStartColorMax(const FVector& InMax);
    void SetStartAlpha(const FRawDistribution<float>& InAlpha);
    void SetStartAlphaMin(float InMin);
    void SetStartAlphaMax(float InMax);
    void SetClampAlpha(bool bClamp);

    // ========================================================================
    // 헬퍼 함수
    // ========================================================================

    // RGB 색상 범위 설정 (0~1 또는 HDR 값)
    void SetColorRange(const FVector& InMin, const FVector& InMax);

    // 고정 색상 설정 (랜덤 없음)
    void SetFixedColor(const FVector& InColor, float InAlpha = 1.0f);

    // FLinearColor로 설정
    void SetFixedColor(const FLinearColor& InColor);

    // 알파 범위 설정
    void SetAlphaRange(float InMin, float InMax);

private:
    // ========================================================================
    // 색상 관련 프로퍼티
    // ========================================================================

    // ------------------------------------------------------------------------
    // StartColor (초기 색상)
    // ------------------------------------------------------------------------
    // 파티클의 RGB 색상입니다.
    //
    // FVector로 표현:
    // - X = Red (0.0 ~ 1.0, HDR은 1.0 초과 가능)
    // - Y = Green
    // - Z = Blue
    //
    // Distribution의 Mode에 따라:
    // - Uniform 모드: Min~Max 사이에서 랜덤 색상
    // - Curve 모드: EmitterTime에 따라 색상이 변화
    //
    // 기본값: (1, 1, 1) = 흰색
    //
    // 예시:
    // - 불꽃: (1, 0.5, 0) ~ (1, 0.8, 0.2) 주황~노랑
    // - 연기: (0.3, 0.3, 0.3) ~ (0.5, 0.5, 0.5) 회색
    // - 마법: (0.5, 0, 1) ~ (1, 0.5, 1) 보라~핑크
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[색상]", Tooltip="파티클의 RGB 색상 (0~1 범위)")
    FRawDistribution<FVector> StartColor{};

    // ------------------------------------------------------------------------
    // StartAlpha (초기 알파)
    // ------------------------------------------------------------------------
    // 파티클의 투명도입니다.
    //
    // 값 범위:
    // - 0.0 = 완전 투명
    // - 1.0 = 완전 불투명
    //
    // bClampAlpha가 true면 0~1로 클램핑됨
    //
    // 기본값: 1.0 (불투명)
    //
    // 예시:
    // - 유령 효과: 0.3 ~ 0.5 (반투명)
    // - 발광 파티클: 1.0 (불투명, 셰이더에서 블렌딩)
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[색상]", Tooltip="파티클의 알파값 (0~1)")
    FRawDistribution<float> StartAlpha{};

    // ------------------------------------------------------------------------
    // bClampAlpha (알파 클램핑)
    // ------------------------------------------------------------------------
    // true: 알파값을 0~1 범위로 클램핑
    // false: 제한 없음 (HDR 알파 등)
    //
    // 기본값: true
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[색상]", Tooltip="알파값을 0~1 범위로 제한")
    bool bClampAlpha = true;
};
