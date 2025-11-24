#pragma once

#include "Actor.h"
#include "ParticleEventTypes.h"
#include "AParticleEventManager.generated.h"

class UParticleSystemComponent;

/**
 * Delegate types for particle events
 */
using FOnParticleCollision = std::function<void(const FParticleEventCollideData&)>;
using FOnParticleDeath = std::function<void(const FParticleEventDeathData&)>;
using FOnParticleSpawn = std::function<void(const FParticleEventSpawnData&)>;

/**
 * Event receiver entry - defines how to handle received events
 */
struct FParticleEventReceiverEntry
{
    // Name of the event to receive
    FString EventName;

    // Type of event
    EParticleEventType EventType = EParticleEventType::Any;

    // Whether this receiver is enabled
    bool bEnabled = true;

    // Callback for collision events
    FOnParticleCollision OnCollision;

    // Callback for death events
    FOnParticleDeath OnDeath;

    // Callback for spawn events
    FOnParticleSpawn OnSpawn;
};

/**
 * Particle Event Manager - manages and dispatches particle events
 * This actor receives events from particle systems and routes them to listeners
 */
UCLASS()
class AParticleEventManager : public AActor
{
public:
    AParticleEventManager();
    virtual ~AParticleEventManager() = default;

    GENERATED_REFLECTION_BODY()

    // AActor interface
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // Register a particle system component to receive events from
    void RegisterParticleSystem(UParticleSystemComponent* Component);

    // Unregister a particle system component
    void UnregisterParticleSystem(UParticleSystemComponent* Component);

    // Add event receiver
    void AddEventReceiver(const FParticleEventReceiverEntry& Receiver);

    // Remove event receiver by name
    void RemoveEventReceiver(const FString& EventName);

    // Process all pending events from registered particle systems
    void ProcessEvents();

    // Handle collision event
    void HandleCollisionEvent(const FParticleEventCollideData& Event);

    // Handle death event
    void HandleDeathEvent(const FParticleEventDeathData& Event);

    // Handle spawn event
    void HandleSpawnEvent(const FParticleEventSpawnData& Event);

    // Get registered particle systems
    const TArray<UParticleSystemComponent*>& GetRegisteredSystems() const { return RegisteredSystems; }

    // Get event receivers
    const TArray<FParticleEventReceiverEntry>& GetEventReceivers() const { return EventReceivers; }

    // Enable/disable event processing
    void SetEnabled(bool bEnable) { bIsEnabled = bEnable; }
    bool IsEnabled() const { return bIsEnabled; }

    // Static instance getter (singleton pattern for easy access)
    static AParticleEventManager* GetInstance(UWorld* World);

private:
    // Registered particle system components
    TArray<UParticleSystemComponent*> RegisteredSystems;

    // Event receiver entries
    TArray<FParticleEventReceiverEntry> EventReceivers;

    // Whether event processing is enabled
    UPROPERTY(EditAnywhere, Category="Events")
    bool bIsEnabled = true;

    // Whether to automatically process events each tick
    UPROPERTY(EditAnywhere, Category="Events")
    bool bAutoProcessEvents = true;

    // Dispatch event to matching receivers
    template<typename EventType>
    void DispatchEvent(const EventType& Event, EParticleEventType Type);
};
