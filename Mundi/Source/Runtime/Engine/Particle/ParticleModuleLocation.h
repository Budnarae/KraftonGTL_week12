#pragma once

#include "UParticleModuleLocation.generated.h"
#include "Statistics.h"

struct FBaseParticle;

UCLASS(DisplayName = "파티클 모듈 로케이션", Description = "파티클의 초기 스폰 위치를 지정합니다.")
class UParticleModuleLocation : public UParticleModule
{
public:
    UParticleModuleLocation() = default;
    ~UParticleModuleLocation() = default;

    GENERATED_REFLECTION_BODY()

        void Spawn(FParticleContext& Context, float EmitterTime) override;
    /* Spawn 전용 모듈이므로 Update override를 구현하지 않음 */

    // -------------------------------------------
    // Getters
    // -------------------------------------------
    const FRawDistribution<FVector>& GetDistribution() const;
    FVector GetDistributionMin() const;
    FVector GetDistributionMax() const;
    float GetDistributeOverNPoints() const;
    float GetDistributionThreshold() const;

    // -------------------------------------------
    // Setters
    // -------------------------------------------
    void SetDistribution(const FRawDistribution<FVector>& InDistribution);
    void SetDistributionMin(const FVector& InMin);
    void SetDistributionMax(const FVector& InMax);
    void SetDistributeOverNPoints(float InValue);
    void SetDistributionThreshold(float InValue);

    // -------------------------------------------
    // 헬퍼 함수
    // -------------------------------------------
    // Distribution의 Min과 Max를 한 번에 설정
    void SetDistributionRange(const FVector& InMin, const FVector& InMax);

    // 박스 형태의 스폰 영역 설정 (중심점과 반경 기반)
    void SetDistributionBox(const FVector& Center, const FVector& HalfExtent);

    // 구체 형태의 스폰 영역 설정 (균일한 반경)
    void SetDistributionSphere(const FVector& Center, float Radius);

private:
    UPROPERTY(EditAnywhere, Category = "[위치]", Tooltip = "스폰 위치의 최소/최대 범위")
    FRawDistribution<FVector> Distribution{};

    UPROPERTY(EditAnywhere, Category = "[위치]", Tooltip = "균일 분산에 사용할 포인트 개수")
    float DistributeOverNPoints{};

    UPROPERTY(EditAnywhere, Category = "[위치]", Range = "0.0, 1.0", Tooltip = "균일 분산 사용 임계값")
    float DistributionThreshold{};
};