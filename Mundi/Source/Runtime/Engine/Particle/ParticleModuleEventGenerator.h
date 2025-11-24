#pragma once

#include "ParticleModule.h"
#include "ParticleData.h"
#include "ParticleEventTypes.h"
#include "UParticleModuleEventGenerator.generated.h"

class UParticleSystemComponent;
struct FParticleContext;

/**
 * Event generator entry - defines what events to generate
 */
struct FParticleEventGeneratorEntry
{
    // Type of event to generate
    EParticleEventType EventType = EParticleEventType::Any;

    // Name of the event (for filtering by receivers)
    FString EventName;

    // Whether this event is enabled
    bool bEnabled = true;

    FParticleEventGeneratorEntry()
        : EventType(EParticleEventType::Any)
        , bEnabled(true)
    {}
};

/**
 * Event generator module - generates events for particle system
 * This module defines what events the emitter should produce
 */
UCLASS(DisplayName="파티클 이벤트 생성기", Description="파티클 시스템에서 이벤트를 생성합니다.")
class UParticleModuleEventGenerator : public UParticleModule
{
public:
    UParticleModuleEventGenerator();
    ~UParticleModuleEventGenerator() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // Generate spawn event
    void GenerateSpawnEvent(
        int32 ParticleIndex,
        const FVector& Location,
        const FVector& Velocity,
        float EmitterTime,
        UParticleSystemComponent* Component
    );

    // Generate death event
    void GenerateDeathEvent(
        int32 ParticleIndex,
        const FVector& Location,
        const FVector& Velocity,
        float EmitterTime,
        UParticleSystemComponent* Component
    );

    // Check if event type should be generated
    bool ShouldGenerateEvent(EParticleEventType EventType) const;

    // Add event generator entry
    void AddEventEntry(const FParticleEventGeneratorEntry& Entry);

    // Remove event entry by name
    void RemoveEventEntry(const FString& EventName);

    // Get all event entries
    const TArray<FParticleEventGeneratorEntry>& GetEventEntries() const { return EventGeneratorEntries; }

    // Enable/disable spawn events
    void SetGenerateSpawnEvents(bool bEnable) { bGenerateSpawnEvents = bEnable; }
    bool GetGenerateSpawnEvents() const { return bGenerateSpawnEvents; }

    // Enable/disable death events
    void SetGenerateDeathEvents(bool bEnable) { bGenerateDeathEvents = bEnable; }
    bool GetGenerateDeathEvents() const { return bGenerateDeathEvents; }

    // Enable/disable collision events
    void SetGenerateCollisionEvents(bool bEnable) { bGenerateCollisionEvents = bEnable; }
    bool GetGenerateCollisionEvents() const { return bGenerateCollisionEvents; }

private:
    // Event generator entries
    TArray<FParticleEventGeneratorEntry> EventGeneratorEntries;

    // Quick access flags for common event types
    UPROPERTY(EditAnywhere, Category="Events")
    bool bGenerateSpawnEvents = false;

    UPROPERTY(EditAnywhere, Category="Events")
    bool bGenerateDeathEvents = false;

    UPROPERTY(EditAnywhere, Category="Events")
    bool bGenerateCollisionEvents = true;

    // Event names for generated events
    UPROPERTY(EditAnywhere, Category="Events")
    FString SpawnEventName = "ParticleSpawn";

    UPROPERTY(EditAnywhere, Category="Events")
    FString DeathEventName = "ParticleDeath";

    UPROPERTY(EditAnywhere, Category="Events")
    FString CollisionEventName = "ParticleCollision";
};
