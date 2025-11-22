#pragma once

#include "ParticleModule.h"
#include "Material.h"
#include "UParticleModuleRequired.generated.h"

UCLASS(DisplayName="필수 파티클 모듈", Description="파티클 렌더링에 필수적인 요소들을 포함하는 모듈입니다.")
class UParticleModuleRequired : public UParticleModule
{
public:
    UParticleModuleRequired();
    ~UParticleModuleRequired() = default;

    GENERATED_REFLECTION_BODY()

    // Getters
    UMaterial* GetMaterial() const { return Material; }
    FVector GetEmitterOrigin() const { return EmitterOrigin; }
    FQuat GetEmitterRotation() const { return EmitterRotation; }
    float GetEmitterDuration() const { return EmitterDuration; }
    float GetSpawnRate() const { return SpawnRate; }
    float GetEmitterDelay() const { return EmitterDelay; }

    // Setters
    void SetMaterial(UMaterial* InMaterial) { Material = InMaterial; }
    void SetEmitterOrigin(const FVector& InOrigin) { EmitterOrigin = InOrigin; }
    void SetEmitterRotation(const FQuat& InRotation) { EmitterRotation = InRotation; }
    void SetEmitterDuration(float InDuration) { EmitterDuration = InDuration; }
    void SetSpawnRate(float InSpawnRate) { SpawnRate = InSpawnRate; }
    void SetEmitterDelay(float InDelay) { EmitterDelay = InDelay; }

private:
    UPROPERTY(EditAnywhere, Category="Assets")
    UMaterial* Material{};

    UPROPERTY(EditAnywhere, Category="Basic")
    FVector EmitterOrigin{};

    UPROPERTY(EditAnywhere, Category="Basic")
    FQuat EmitterRotation{};

    UPROPERTY(EditAnywhere, Category="Basic")
    float EmitterDuration{};

    UPROPERTY(EditAnywhere, Category="Basic")
    float SpawnRate{};

    /** The array of burst entries.												*/
    // UPROPERTY(EditAnywhere, Category="Array")
    // TArray<FParticleBurst> BurstList;

    UPROPERTY(EditAnywhere, Category="Basic")
    float EmitterDelay{};

public:
    UPROPERTY(EditAnywhere, Category = "Basic")
    float Test = 0;
};