#include "pch.h"
#include "ParticleModuleEventGenerator.h"
#include "ParticleData.h"
#include "ParticleSystemComponent.h"

UParticleModuleEventGenerator::UParticleModuleEventGenerator()
    : UParticleModule(0) // No payload needed
{
}

void UParticleModuleEventGenerator::Spawn(FBaseParticle* Particle, float EmitterTime)
{
    // Spawn events are generated externally when particles are created
    // This is just a placeholder for the module interface
}

void UParticleModuleEventGenerator::Update(FBaseParticle* Particle, float DeltaTime)
{
    // Update events (like death) are generated when conditions are met
    // This is just a placeholder for the module interface
}

void UParticleModuleEventGenerator::GenerateSpawnEvent(
    int32 ParticleIndex,
    const FVector& Location,
    const FVector& Velocity,
    float EmitterTime,
    UParticleSystemComponent* Component)
{
    if (!bGenerateSpawnEvents || !Component)
        return;

    FParticleEventSpawnData SpawnEvent;
    SpawnEvent.ParticleSystemComponent = Component;
    SpawnEvent.ParticleIndex = ParticleIndex;
    SpawnEvent.Location = Location;
    SpawnEvent.Velocity = Velocity;
    SpawnEvent.Direction = Velocity.IsZero() ? FVector(1.0f, 0.0f, 0.0f) : Velocity.GetSafeNormal();
    SpawnEvent.EmitterTime = EmitterTime;
    SpawnEvent.EventName = SpawnEventName;

    Component->AddSpawnEvent(SpawnEvent);
}

void UParticleModuleEventGenerator::GenerateDeathEvent(
    int32 ParticleIndex,
    const FVector& Location,
    const FVector& Velocity,
    float EmitterTime,
    UParticleSystemComponent* Component)
{
    if (!bGenerateDeathEvents || !Component)
        return;

    FParticleEventDeathData DeathEvent;
    DeathEvent.ParticleSystemComponent = Component;
    DeathEvent.ParticleIndex = ParticleIndex;
    DeathEvent.Location = Location;
    DeathEvent.Velocity = Velocity;
    DeathEvent.Direction = Velocity.IsZero() ? FVector(1.0f, 0.0f, 0.0f) : Velocity.GetSafeNormal();
    DeathEvent.EmitterTime = EmitterTime;
    DeathEvent.EventName = DeathEventName;

    Component->AddDeathEvent(DeathEvent);
}

bool UParticleModuleEventGenerator::ShouldGenerateEvent(EParticleEventType EventType) const
{
    switch (EventType)
    {
    case EParticleEventType::Spawn:
        return bGenerateSpawnEvents;
    case EParticleEventType::Death:
        return bGenerateDeathEvents;
    case EParticleEventType::Collision:
        return bGenerateCollisionEvents;
    case EParticleEventType::Any:
        return true;
    default:
        break;
    }

    // Check custom entries
    for (const FParticleEventGeneratorEntry& Entry : EventGeneratorEntries)
    {
        if (Entry.EventType == EventType && Entry.bEnabled)
        {
            return true;
        }
    }

    return false;
}

void UParticleModuleEventGenerator::AddEventEntry(const FParticleEventGeneratorEntry& Entry)
{
    EventGeneratorEntries.Add(Entry);
}

void UParticleModuleEventGenerator::RemoveEventEntry(const FString& EventName)
{
    for (int32 i = EventGeneratorEntries.Num() - 1; i >= 0; --i)
    {
        if (EventGeneratorEntries[i].EventName == EventName)
        {
            EventGeneratorEntries.RemoveAt(i);
        }
    }
}
