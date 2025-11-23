#pragma once

#include "Vector.h"
#include "Name.h"

// Forward declarations
class AActor;
class UPrimitiveComponent;
class UParticleSystemComponent;

/**
 * Base structure for all particle event data
 */
struct FParticleEventData
{
    // The type of event
    int32 Type{};

    // The name of the event (for filtering)
    FString EventName{};

    // The time at which the event occurred
    float EmitterTime{};

    // The particle system component that generated this event
    UParticleSystemComponent* ParticleSystemComponent{};

    // The location where the event occurred
    FVector Location{};

    // The velocity of the particle at the time of the event
    FVector Velocity{};

    // The direction of the particle (normalized velocity)
    FVector Direction{};

    FParticleEventData()
        : Type(0)
        , EmitterTime(0.0f)
        , ParticleSystemComponent(nullptr)
        , Location(FVector::Zero())
        , Velocity(FVector::Zero())
        , Direction(FVector::Zero())
    {}
};

/**
 * Collision event data - contains information about a particle collision
 */
struct FParticleEventCollideData : public FParticleEventData
{
    // The particle index that collided
    int32 ParticleIndex{};

    // The normal of the surface that was hit
    FVector Normal{};

    // The time along the particle's path where the collision occurred (0-1)
    float Time{};

    // The item that was collided with
    int32 Item{};

    // The bone name (for skeletal mesh collisions)
    FString BoneName{};

    // The actor that was hit
    AActor* HitActor{};

    // The component that was hit
    UPrimitiveComponent* HitComponent{};

    // Physical material properties
    float Friction{};
    float Restitution{};

    FParticleEventCollideData()
        : FParticleEventData()
        , ParticleIndex(-1)
        , Normal(FVector(0.0f, 0.0f, 1.0f))
        , Time(0.0f)
        , Item(-1)
        , HitActor(nullptr)
        , HitComponent(nullptr)
        , Friction(0.5f)
        , Restitution(0.5f)
    {
        Type = 1; // Collision event type
    }
};

/**
 * Death event data - contains information about a particle death
 */
struct FParticleEventDeathData : public FParticleEventData
{
    // The particle index that died
    int32 ParticleIndex{};

    FParticleEventDeathData()
        : FParticleEventData()
        , ParticleIndex(-1)
    {
        Type = 2; // Death event type
    }
};

/**
 * Spawn event data - contains information about a particle spawn
 */
struct FParticleEventSpawnData : public FParticleEventData
{
    // The particle index that was spawned
    int32 ParticleIndex{};

    FParticleEventSpawnData()
        : FParticleEventData()
        , ParticleIndex(-1)
    {
        Type = 3; // Spawn event type
    }
};

/**
 * Burst event data - contains information about a burst spawn
 */
struct FParticleEventBurstData : public FParticleEventData
{
    // Number of particles spawned in the burst
    int32 ParticleCount{};

    FParticleEventBurstData()
        : FParticleEventData()
        , ParticleCount(0)
    {
        Type = 4; // Burst event type
    }
};

/**
 * Event types enumeration
 */
enum class EParticleEventType : uint8
{
    Any = 0,
    Collision = 1,
    Death = 2,
    Spawn = 3,
    Burst = 4
};
