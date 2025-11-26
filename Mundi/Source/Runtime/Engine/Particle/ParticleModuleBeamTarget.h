#pragma once

#include "ParticleModule.h"
#include "Vector.h"
#include "UParticleModuleBeamTarget.generated.h"

/**
 * Beam target method - how the target point is determined
 */
enum class EBeamTargetMethod : uint8
{
    Static,     // Use fixed TargetPoint (local to emitter)
    Emitter,    // Use emitter location + offset
    Actor,      // Find actor by name and use its location
    Socket,     // Find actor's socket/bone location
    UserSet     // Set programmatically at runtime
};

/**
 * Module that defines how the beam's target point is determined.
 * If this module is not present, TypeDataBeam will use its internal TargetPoint as fallback.
 */
UCLASS(DisplayName="빔 타겟", Description="빔의 끝점(타겟)을 어떻게 결정할지 정의합니다.")
class UParticleModuleBeamTarget : public UParticleModule
{
public:
    UParticleModuleBeamTarget();
    virtual ~UParticleModuleBeamTarget() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // Resolve the actual target point based on current settings
    // Returns true if successfully resolved, false if fallback should be used
    bool ResolveTargetPoint(class UParticleSystemComponent* Owner, FVector& OutTargetPoint) const;

    // Getters
    EBeamTargetMethod GetTargetMethod() const { return TargetMethod; }
    FVector GetTargetPoint() const { return TargetPoint; }
    FVector GetTargetOffset() const { return TargetOffset; }
    FName GetTargetActorName() const { return TargetActorName; }
    FName GetTargetActorTag() const { return TargetActorTag; }
    FName GetTargetSocketName() const { return TargetSocketName; }

    // Setters
    void SetTargetMethod(EBeamTargetMethod Value) { TargetMethod = Value; }
    void SetTargetPoint(const FVector& Value) { TargetPoint = Value; }
    void SetTargetOffset(const FVector& Value) { TargetOffset = Value; }
    void SetTargetActorName(const FName& Value) { TargetActorName = Value; }
    void SetTargetActorTag(const FName& Value) { TargetActorTag = Value; }
    void SetTargetSocketName(const FName& Value) { TargetSocketName = Value; }

    // Runtime API for UserSet method
    void SetUserTargetPoint(const FVector& Value) { UserTargetPoint = Value; }
    FVector GetUserTargetPoint() const { return UserTargetPoint; }

private:
    // How to determine target point
    UPROPERTY(EditAnywhere, Category="Target|Method")
    EBeamTargetMethod TargetMethod = EBeamTargetMethod::Static;

    // Static method: fixed target point (local to emitter)
    UPROPERTY(EditAnywhere, Category="Target|Static")
    FVector TargetPoint = FVector(100.0f, 0.0f, 0.0f);

    // Offset applied to all methods
    UPROPERTY(EditAnywhere, Category="Target|Offset")
    FVector TargetOffset = FVector::Zero();

    // Actor method: find actor by name
    UPROPERTY(EditAnywhere, Category="Target|Actor")
    FName TargetActorName;

    // Actor method: find actor by tag (used if ActorName is empty)
    UPROPERTY(EditAnywhere, Category="Target|Actor")
    FName TargetActorTag;

    // Socket method: bone/socket name on the target actor
    UPROPERTY(EditAnywhere, Category="Target|Socket")
    FName TargetSocketName;

    // UserSet method: programmatically set at runtime (not serialized)
    FVector UserTargetPoint = FVector::Zero();
};
