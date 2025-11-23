#pragma once

#include "ParticleModule.h"
#include "Material.h"
#include "UParticleModuleRequired.generated.h"

UCLASS(DisplayName="필수 파티클 모듈", Description="파티클 렌더링에 필수적인 요소들을 포함하는 모듈입니다.")
class UParticleModuleRequired : public UParticleModule
{
public:
    // RequiredModule이 사용하는 페이로드 크기
    // EmitterOrigin(12) + EmitterRotation(16) + EmitterDuration(4) + SpawnRate(4) + EmitterDelay(4) + LifeTime(4) = 44 바이트
    // 16바이트 정렬을 위해 48바이트로 설정
    static constexpr int32 REQUIRED_MODULE_PAYLOAD_SIZE = 48;

    UParticleModuleRequired();
    ~UParticleModuleRequired() = default;

    GENERATED_REFLECTION_BODY()

    // 파티클 모듈 가상 함수 오버라이드
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // Getters
    UMaterial* GetMaterial() const { return Material; }
    FVector GetEmitterOrigin() const { return EmitterOrigin; }
    FQuat GetEmitterRotation() const { return EmitterRotation; }
    float GetEmitterDuration() const { return EmitterDuration; }
    float GetSpawnRate() const { return SpawnRate; }
    float GetEmitterDelay() const { return EmitterDelay; }
    float GetLifeTime() const { return LifeTime; }

    // Setters
    void SetMaterial(UMaterial* InMaterial) { Material = InMaterial; }
    void SetEmitterOrigin(const FVector& InOrigin) { EmitterOrigin = InOrigin; }
    void SetEmitterRotation(const FQuat& InRotation) { EmitterRotation = InRotation; }
    void SetEmitterDuration(float InDuration) { EmitterDuration = InDuration; }
    void SetSpawnRate(float InSpawnRate) { SpawnRate = InSpawnRate; }
    void SetEmitterDelay(float InDelay) { EmitterDelay = InDelay; }
    void SetLifeTime(float InLifeTime) { LifeTime = InLifeTime; }

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
    UPROPERTY(EditAnywhere, Category="Basic")
    float LifeTime = 3.f;
};