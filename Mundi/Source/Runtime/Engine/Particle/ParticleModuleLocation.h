#pragma once

#include "UParticleModuleLocation.generated.h"

struct FBaseParticle;

UCLASS(DisplayName="파티클 모듈 로케이션", Description="파티클의 초기 스폰 위치를 지정합니다.")
class UParticleModuleLocation : public UObject
{
public:
    UParticleModuleLocation() = default;
    ~UParticleModuleLocation() = default;

    GENERATED_REFLECTION_BODY()

    void Spawn(FBaseParticle* Particle, float EmitterTime);
    /* Spawn 전용 모듈이므로 override를 구현하지 않음 */

private:
    
};