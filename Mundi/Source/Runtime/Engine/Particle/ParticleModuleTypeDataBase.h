#pragma once

#include "ParticleModule.h"
#include "UParticleModuleTypeDataBase.generated.h"

// EDynamicEmitterType definition
enum EDynamicEmitterType : int32
{
    EDET_Sprite = 0,
    EDET_Mesh = 1,
    EDET_Beam = 2,
    EDET_Ribbon = 3
};

/**
 * Base class for TypeData modules that define emitter rendering type
 * (Sprite, Mesh, Beam, Ribbon, etc.)
 */
UCLASS(DisplayName="파티클 타입 데이터 베이스", Description="에미터의 렌더링 타입을 정의하는 기본 클래스입니다.")
class UParticleModuleTypeDataBase : public UParticleModule
{
public:
    UParticleModuleTypeDataBase();
    virtual ~UParticleModuleTypeDataBase() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // TypeData interface
    virtual EDynamicEmitterType GetEmitterType() const { return EDynamicEmitterType::EDET_Sprite; }

    // Check if this TypeData requires special rendering
    virtual bool RequiresSpecialRendering() const { return false; }
    void SetActive(const bool InActive) override { bActive = InActive; }

};
