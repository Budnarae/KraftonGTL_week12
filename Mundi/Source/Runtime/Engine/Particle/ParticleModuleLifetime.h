#pragma once

#include "UParticleModuleLifetime.generated.h"
#include "Statistics.h"

struct FBaseParticle;

// ============================================================================
// UParticleModuleLifetime
// ============================================================================
// 파티클의 수명을 설정하는 모듈입니다.
//
// 언리얼 엔진의 UParticleModuleLifetime을 참고하여 구현되었습니다.
//
// 주요 기능:
// 1. Lifetime Distribution으로 파티클 수명 결정
// 2. EmitterTime 기반 시간에 따른 수명 변화 가능
// 3. OneOverMaxLifetime을 설정하여 RelativeTime 계산 지원
//
// RelativeTime의 역할:
// - 0.0: 파티클이 막 스폰됨
// - 0.5: 수명의 절반이 경과
// - 1.0: 수명이 다 됨 (제거 대상)
//
// OneOverMaxLifetime을 사용하는 이유:
// - RelativeTime 계산: RelativeTime += DeltaTime * OneOverMaxLifetime
// - 나눗셈 대신 곱셈으로 성능 최적화
// ============================================================================
UCLASS(DisplayName="파티클 모듈 수명", Description="파티클의 수명을 지정합니다.")
class UParticleModuleLifetime : public UParticleModule
{
public:
    UParticleModuleLifetime();
    ~UParticleModuleLifetime() = default;

    GENERATED_REFLECTION_BODY()

    // ========================================================================
    // UParticleModule 인터페이스 구현
    // ========================================================================

    // Spawn: 파티클이 생성될 때 호출됨
    // - 파티클의 수명(OneOverMaxLifetime)을 설정
    // - 초기 RelativeTime을 설정
    void Spawn(FParticleContext& Context, float EmitterTime) override;

    // ========================================================================
    // Getters
    // ========================================================================
    const FRawDistribution<float>& GetLifetime() const;
    float GetLifetimeMin() const;
    float GetLifetimeMax() const;
    float GetMaxLifetime() const;

    // ========================================================================
    // Setters
    // ========================================================================
    void SetLifetime(const FRawDistribution<float>& InLifetime);
    void SetLifetimeMin(float InMin);
    void SetLifetimeMax(float InMax);

    // ========================================================================
    // 헬퍼 함수
    // ========================================================================

    // 수명 범위를 한 번에 설정
    void SetLifetimeRange(float InMin, float InMax);

    // 고정 수명 설정 (랜덤 없음)
    void SetFixedLifetime(float InLifetime);

    // EmitterTime에 따른 수명 값 반환 (외부 조회용)
    float GetLifetimeValue(float EmitterTime) const;

private:
    // ========================================================================
    // 수명 관련 프로퍼티
    // ========================================================================

    // ------------------------------------------------------------------------
    // Lifetime (파티클 수명)
    // ------------------------------------------------------------------------
    // 파티클이 존재할 시간(초)입니다.
    //
    // Distribution의 Mode에 따라:
    // - Uniform 모드: Min~Max 사이에서 랜덤 수명
    // - Curve 모드: EmitterTime에 따라 수명이 변화
    //
    // 기본값: 1.0초
    //
    // 예시:
    // - 불꽃: 0.1~0.3초 (짧은 수명)
    // - 연기: 3~5초 (긴 수명)
    // - 잔해: 5~10초 (매우 긴 수명)
    //
    // 주의:
    // - 수명이 0이면 파티클이 즉시 제거됨
    // - 음수 수명은 0으로 처리됨
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[수명]", Tooltip="파티클의 수명 (초)")
    FRawDistribution<float> Lifetime{};
};
