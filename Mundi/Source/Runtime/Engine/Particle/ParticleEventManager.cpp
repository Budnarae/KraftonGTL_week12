#include "pch.h"
#include "ParticleEventManager.h"
#include "ParticleSystemComponent.h"
#include "World.h"

AParticleEventManager::AParticleEventManager()
{
}

void AParticleEventManager::BeginPlay()
{
    AActor::BeginPlay();
}

void AParticleEventManager::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);

    if (bIsEnabled && bAutoProcessEvents)
    {
        ProcessEvents();
    }
}

void AParticleEventManager::RegisterParticleSystem(UParticleSystemComponent* Component)
{
    if (!Component)
        return;

    // Avoid duplicates
    if (!RegisteredSystems.Contains(Component))
    {
        RegisteredSystems.Add(Component);
    }
}

void AParticleEventManager::UnregisterParticleSystem(UParticleSystemComponent* Component)
{
    RegisteredSystems.Remove(Component);
}

void AParticleEventManager::AddEventReceiver(const FParticleEventReceiverEntry& Receiver)
{
    EventReceivers.Add(Receiver);
}

void AParticleEventManager::RemoveEventReceiver(const FString& EventName)
{
    for (int32 i = EventReceivers.Num() - 1; i >= 0; --i)
    {
        if (EventReceivers[i].EventName == EventName)
        {
            EventReceivers.RemoveAt(i);
        }
    }
}

void AParticleEventManager::ProcessEvents()
{
    if (!bIsEnabled)
        return;

    // Process events from all registered particle systems
    for (UParticleSystemComponent* Component : RegisteredSystems)
    {
        if (!Component)
            continue;

        // Process collision events
        const TArray<FParticleEventCollideData>& CollisionEvents = Component->GetCollisionEvents();
        for (const FParticleEventCollideData& Event : CollisionEvents)
        {
            HandleCollisionEvent(Event);
        }

        // Process death events
        const TArray<FParticleEventDeathData>& DeathEvents = Component->GetDeathEvents();
        for (const FParticleEventDeathData& Event : DeathEvents)
        {
            HandleDeathEvent(Event);
        }

        // Process spawn events
        const TArray<FParticleEventSpawnData>& SpawnEvents = Component->GetSpawnEvents();
        for (const FParticleEventSpawnData& Event : SpawnEvents)
        {
            HandleSpawnEvent(Event);
        }

        // Clear processed events
        Component->ClearAllEvents();
    }
}

void AParticleEventManager::HandleCollisionEvent(const FParticleEventCollideData& Event)
{
    for (FParticleEventReceiverEntry& Receiver : EventReceivers)
    {
        if (!Receiver.bEnabled)
            continue;

        // Check if receiver wants this event type
        if (Receiver.EventType != EParticleEventType::Any &&
            Receiver.EventType != EParticleEventType::Collision)
            continue;

        // Check event name filter (empty name means accept all)
        if (!Receiver.EventName.empty() && Receiver.EventName != Event.EventName)
            continue;

        // Call the callback if set
        if (Receiver.OnCollision)
        {
            Receiver.OnCollision(Event);
        }
    }
}

void AParticleEventManager::HandleDeathEvent(const FParticleEventDeathData& Event)
{
    for (FParticleEventReceiverEntry& Receiver : EventReceivers)
    {
        if (!Receiver.bEnabled)
            continue;

        // Check if receiver wants this event type
        if (Receiver.EventType != EParticleEventType::Any &&
            Receiver.EventType != EParticleEventType::Death)
            continue;

        // Check event name filter
        if (!Receiver.EventName.empty() && Receiver.EventName != Event.EventName)
            continue;

        // Call the callback if set
        if (Receiver.OnDeath)
        {
            Receiver.OnDeath(Event);
        }
    }
}

void AParticleEventManager::HandleSpawnEvent(const FParticleEventSpawnData& Event)
{
    for (FParticleEventReceiverEntry& Receiver : EventReceivers)
    {
        if (!Receiver.bEnabled)
            continue;

        // Check if receiver wants this event type
        if (Receiver.EventType != EParticleEventType::Any &&
            Receiver.EventType != EParticleEventType::Spawn)
            continue;

        // Check event name filter
        if (!Receiver.EventName.empty() && Receiver.EventName != Event.EventName)
            continue;

        // Call the callback if set
        if (Receiver.OnSpawn)
        {
            Receiver.OnSpawn(Event);
        }
    }
}

AParticleEventManager* AParticleEventManager::GetInstance(UWorld* World)
{
    if (!World)
        return nullptr;

    // Find existing event manager in the world
    const TArray<AActor*>& Actors = World->GetActors();
    for (AActor* Actor : Actors)
    {
        AParticleEventManager* Manager = Cast<AParticleEventManager>(Actor);
        if (Manager)
        {
            return Manager;
        }
    }

    // No manager found - could create one here if needed
    return nullptr;
}
