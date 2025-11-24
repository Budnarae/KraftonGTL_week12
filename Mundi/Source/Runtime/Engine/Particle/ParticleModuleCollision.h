#pragma once

#include "ParticleModule.h"
#include "ParticleData.h"
#include "UParticleModuleCollision.generated.h"

class UParticleSystemComponent;
class UWorld;
struct FBaseParticle;
struct FParticleContext;

/**
 * Collision response type
 */
enum class EParticleCollisionResponse : uint8
{
    Bounce,     // Reflect velocity
    Stop,       // Stop particle movement
    Kill        // Kill the particle
};

/**
 * Collision module - handles particle collision with scene geometry
 */
UCLASS(DisplayName="파티클 충돌 모듈", Description="파티클과 씬 지오메트리의 충돌을 처리합니다.")
class UParticleModuleCollision : public UParticleModule
{
public:
    UParticleModuleCollision();
    ~UParticleModuleCollision() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // Getters
    float GetDampingFactor() const { return DampingFactor; }
    float GetDampingFactorRotation() const { return DampingFactorRotation; }
    float GetFriction() const { return Friction; }
    float GetRestitution() const { return Restitution; }
    float GetCollisionRadius() const { return CollisionRadius; }
    int32 GetMaxCollisions() const { return MaxCollisions; }
    bool GetApplyPhysics() const { return bApplyPhysics; }
    bool GetKillOnCollision() const { return bKillOnCollision; }
    EParticleCollisionResponse GetCollisionResponse() const { return CollisionResponse; }

    // Setters
    void SetDampingFactor(float Value) { DampingFactor = Value; }
    void SetDampingFactorRotation(float Value) { DampingFactorRotation = Value; }
    void SetFriction(float Value) { Friction = Value; }
    void SetRestitution(float Value) { Restitution = Value; }
    void SetCollisionRadius(float Value) { CollisionRadius = Value; }
    void SetMaxCollisions(int32 Value) { MaxCollisions = Value; }
    void SetApplyPhysics(bool Value) { bApplyPhysics = Value; }
    void SetKillOnCollision(bool Value) { bKillOnCollision = Value; }
    void SetCollisionResponse(EParticleCollisionResponse Value) { CollisionResponse = Value; }

private:
    // Collision response settings
    UPROPERTY(EditAnywhere, Category="Collision")
    float DampingFactor = 0.0f;

    UPROPERTY(EditAnywhere, Category="Collision")
    float DampingFactorRotation = 1.0f;

    UPROPERTY(EditAnywhere, Category="Collision")
    float Friction = 0.2f;

    UPROPERTY(EditAnywhere, Category="Collision")
    float Restitution = 0.5f;

    // Collision detection settings
    UPROPERTY(EditAnywhere, Category="Collision")
    float CollisionRadius = 1.0f;

    UPROPERTY(EditAnywhere, Category="Collision")
    int32 MaxCollisions = 0;  // 0 = unlimited

    UPROPERTY(EditAnywhere, Category="Collision")
    bool bApplyPhysics = true;

    UPROPERTY(EditAnywhere, Category="Collision")
    bool bKillOnCollision = false;

    UPROPERTY(EditAnywhere, Category="Collision")
    EParticleCollisionResponse CollisionResponse = EParticleCollisionResponse::Bounce;

    // Internal collision check
    bool CheckCollisionWithWorld(
        const FVector& OldLocation,
        const FVector& NewLocation,
        float Radius,
        UWorld* World,
        FVector& OutHitLocation,
        FVector& OutHitNormal,
        AActor*& OutHitActor,
        UPrimitiveComponent*& OutHitComponent
    );

    // Apply collision response to particle
    void ApplyCollisionResponse(
        FBaseParticle* Particle,
        const FVector& HitLocation,
        const FVector& HitNormal
    );
};
