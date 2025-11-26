#include "pch.h"
#include "ParticleModuleBeamTarget.h"
#include "ParticleSystemComponent.h"
#include "Actor.h"
#include "World.h"

UParticleModuleBeamTarget::UParticleModuleBeamTarget()
    : UParticleModule()
{
}

void UParticleModuleBeamTarget::Spawn(FParticleContext& Context, float EmitterTime)
{
    // Beam target is resolved at render time, not per-particle spawn
}

void UParticleModuleBeamTarget::Update(FParticleContext& Context, float DeltaTime)
{
    // Beam target is resolved at render time, not per-particle update
}

bool UParticleModuleBeamTarget::ResolveTargetPoint(UParticleSystemComponent* Owner, FVector& OutTargetPoint) const
{
    if (!Owner)
    {
        return false;
    }

    switch (TargetMethod)
    {
    case EBeamTargetMethod::Static:
        // Static: use fixed point relative to emitter
        OutTargetPoint = Owner->GetWorldLocation() + TargetPoint + TargetOffset;
        return true;

    case EBeamTargetMethod::Emitter:
        // Emitter: use emitter location + offset
        OutTargetPoint = Owner->GetWorldLocation() + TargetOffset;
        return true;

    case EBeamTargetMethod::Actor:
    {
        // First, check if component has a direct BeamTargetActor reference
        AActor* FoundActor = Owner->GetBeamTargetActor();

        // If not, try to find by name or tag
        if (!FoundActor)
        {
            AActor* OwnerActor = Owner->GetOwner();
            if (OwnerActor)
            {
                UWorld* World = OwnerActor->GetWorld();
                if (World)
                {
                    // Try by name first
                    if (TargetActorName.ToString().size() > 0)
                    {
                        FoundActor = World->FindActorByName(TargetActorName);
                    }
                    // TODO: If name not found, try by tag
                    // if (!FoundActor && !TargetActorTag.IsEmpty())
                    // {
                    //     FoundActor = World->FindActorByTag(TargetActorTag);
                    // }
                }
            }
        }

        if (FoundActor)
        {
            OutTargetPoint = FoundActor->GetActorLocation() + TargetOffset;
            return true;
        }

        // Actor not found - return false to use fallback
        return false;
    }

    case EBeamTargetMethod::Socket:
    {
        // Find actor first
        AActor* FoundActor = Owner->GetBeamTargetActor();

        if (!FoundActor)
        {
            AActor* OwnerActor = Owner->GetOwner();
            if (OwnerActor)
            {
                UWorld* World = OwnerActor->GetWorld();
                if (World && TargetActorName.ToString().size() > 0)
                {
                    FoundActor = World->FindActorByName(TargetActorName);
                }
            }
        }

        if (FoundActor)
        {
            // TODO: Get socket location from skeletal mesh component
            // For now, just use actor location
            // if (auto* SkelMesh = FoundActor->GetComponent<USkeletalMeshComponent>())
            // {
            //     OutTargetPoint = SkelMesh->GetSocketLocation(TargetSocketName) + TargetOffset;
            //     return true;
            // }

            OutTargetPoint = FoundActor->GetActorLocation() + TargetOffset;
            return true;
        }

        return false;
    }

    case EBeamTargetMethod::UserSet:
        // Use programmatically set value
        OutTargetPoint = UserTargetPoint + TargetOffset;
        return true;

    default:
        return false;
    }
}
