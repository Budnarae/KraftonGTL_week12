#pragma once

#include "UParticleModuleSize.generated.h"
#include "Statistics.h"

struct FBaseParticle;

// ============================================================================
// UParticleModuleSize
// ============================================================================
// 파티클의 초기 크기를 설정하는 모듈입니다.
//
// 언리얼 엔진의 UParticleModuleSize를 참고하여 구현되었습니다.
//
// 주요 기능:
// 1. StartSize로 3D 크기 설정 (X, Y, Z)
// 2. 크기는 **누적 방식** (+= 연산)으로 적용
// 3. EmitterTime 기반 시간에 따른 크기 변화 가능
//
// 누적 방식의 장점:
// - 여러 Size 모듈을 스택하여 사용 가능
// - 기본 크기 + 추가 크기로 구성 가능
// - 효과 조합이 유연함
//
// 렌더링에서의 사용:
// - Billboard 셰이더에서 Particle.Size.XY로 쿼드 크기 결정
// - Z는 깊이 또는 3D 파티클에서 사용
// ============================================================================
UCLASS(DisplayName="파티클 모듈 크기", Description="파티클의 초기 크기를 지정합니다.")
class UParticleModuleSize : public UParticleModule
{
public:
    UParticleModuleSize();
    ~UParticleModuleSize() = default;

    GENERATED_REFLECTION_BODY()

    // ========================================================================
    // UParticleModule 인터페이스 구현
    // ========================================================================

    // Spawn: 파티클이 생성될 때 호출됨
    // - Size와 BaseSize에 값을 **누적**
    void Spawn(FParticleContext& Context, float EmitterTime) override;

    // ========================================================================
    // Getters
    // ========================================================================
    const FRawDistribution<FVector>& GetStartSize() const;
    FVector GetStartSizeMin() const;
    FVector GetStartSizeMax() const;

    // ========================================================================
    // Setters
    // ========================================================================
    void SetStartSize(const FRawDistribution<FVector>& InSize);
    void SetStartSizeMin(const FVector& InMin);
    void SetStartSizeMax(const FVector& InMax);

    // ========================================================================
    // 헬퍼 함수
    // ========================================================================

    // 크기 범위 설정
    void SetSizeRange(const FVector& InMin, const FVector& InMax);

    // 고정 크기 설정 (랜덤 없음)
    void SetFixedSize(const FVector& InSize);

    // 균일 크기 설정 (X = Y = Z)
    void SetUniformSize(float InMin, float InMax);

    // 균일 고정 크기 설정
    void SetFixedUniformSize(float InSize);

private:
    // ========================================================================
    // 크기 관련 프로퍼티
    // ========================================================================

    // ------------------------------------------------------------------------
    // StartSize (초기 크기)
    // ------------------------------------------------------------------------
    // 파티클의 3D 크기입니다.
    //
    // FVector로 표현:
    // - X = 가로 크기 (Billboard에서 Width)
    // - Y = 세로 크기 (Billboard에서 Height)
    // - Z = 깊이 (3D 파티클용, Billboard에서는 보통 무시)
    //
    // Distribution의 Mode에 따라:
    // - Uniform 모드: Min~Max 사이에서 랜덤 크기
    // - Curve 모드: EmitterTime에 따라 크기가 변화
    //
    // 기본값: (1, 1, 1) = 1x1x1 유닛
    //
    // 예시:
    // - 불꽃 스파크: (0.05, 0.05, 0.05) ~ (0.1, 0.1, 0.1)
    // - 연기: (0.5, 0.5, 0.5) ~ (2.0, 2.0, 2.0)
    // - 파편: (0.1, 0.1, 0.1) ~ (0.3, 0.3, 0.3)
    //
    // 주의:
    // - 크기 0은 보이지 않음
    // - 음수 크기는 UV 뒤집기 효과 (특수 용도)
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[크기]", Tooltip="파티클의 XYZ 크기")
    FRawDistribution<FVector> StartSize{};
};
