#include "pch.h"
#include "ParticleModuleCollision.h"
#include "ParticleSystemComponent.h"
#include "ParticleEventTypes.h"
#include "World.h"
#include "Actor.h"
#include "ShapeComponent.h"
#include "Collision.h"
#include "OBB.h"

UParticleModuleCollision::UParticleModuleCollision()
    : UParticleModule(0) // No payload needed for basic collision
{
}

void UParticleModuleCollision::Spawn(FParticleContext& Context, float EmitterTime)
{
    // Initialize collision state if needed
    // For now, no special spawn-time initialization required
}

void UParticleModuleCollision::Update(FParticleContext& Context, float DeltaTime)
{
    FBaseParticle* Particle = Context.Particle;
    UParticleSystemComponent* Component = Context.Owner;

    if (!Particle || !Component)
        return;

    // Get World from owner component's actor
    AActor* OwnerActor = Component->GetOwner();
    if (!OwnerActor)
        return;

    UWorld* World = OwnerActor->GetWorld();
    if (!World)
        return;

    // Calculate collision radius from particle size if not explicitly set
    float EffectiveRadius = CollisionRadius;
    if (EffectiveRadius <= 0.0f)
    {
        EffectiveRadius = Particle->Size.X * 0.5f;
    }

    FVector HitLocation;
    FVector HitNormal;
    AActor* HitActor = nullptr;
    UPrimitiveComponent* HitComponent = nullptr;

    // Check collision between old and current location
    if (CheckCollisionWithWorld(
        Particle->OldLocation,
        Particle->Location,
        EffectiveRadius,
        World,
        HitLocation,
        HitNormal,
        HitActor,
        HitComponent,
        OwnerActor))  // Pass owner actor to avoid self-collision
    {
        // Debug log for collision detection
        UE_LOG("[ParticleCollision] Particle %d collided with Actor at (%.2f, %.2f, %.2f)",
            Context.ParticleIndex, HitLocation.X, HitLocation.Y, HitLocation.Z);

        // Generate collision event
        FParticleEventCollideData CollideEvent;
        CollideEvent.ParticleSystemComponent = Component;
        CollideEvent.ParticleIndex = Context.ParticleIndex;
        CollideEvent.Location = HitLocation;
        CollideEvent.Velocity = Particle->Velocity;
        CollideEvent.Direction = Particle->Velocity.IsZero() ? FVector(1.0f, 0.0f, 0.0f) : Particle->Velocity.GetSafeNormal();
        CollideEvent.Normal = HitNormal;
        CollideEvent.HitActor = HitActor;
        CollideEvent.HitComponent = HitComponent;
        CollideEvent.Friction = Friction;
        CollideEvent.Restitution = Restitution;

        // Add event to component
        Component->AddCollisionEvent(CollideEvent);

        // ========================================
        // 충돌한 액터에 대한 처리 (여기에 원하는 동작 추가)
        // ========================================
        if (HitActor)
        {
            // 예시 1: 로그 출력
            UE_LOG("[ParticleCollision] Hit Actor: %s", HitActor->GetName().c_str());

            // 예시 2: 액터에 데미지 주기 (TakeDamage 함수가 있다면)
            // HitActor->TakeDamage(10.0f);

            // 예시 3: 액터 삭제
            // HitActor->Destroy();

            // 예시 4: 액터의 특정 컴포넌트에 힘 가하기
            // if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(HitComponent))
            // {
            //     PrimComp->AddImpulse(Particle->Velocity * 100.0f);
            // }
        }

        // Apply collision response
        if (bKillOnCollision || CollisionResponse == EParticleCollisionResponse::Kill)
        {
            // Mark particle for death by setting lifetime to 0
            Particle->RelativeTime = Particle->LifeTime;
        }
        else
        {
            ApplyCollisionResponse(Particle, HitLocation, HitNormal);
        }
    }
}

bool UParticleModuleCollision::CheckCollisionWithWorld(
    const FVector& OldLocation,
    const FVector& NewLocation,
    float Radius,
    UWorld* World,
    FVector& OutHitLocation,
    FVector& OutHitNormal,
    AActor*& OutHitActor,
    UPrimitiveComponent*& OutHitComponent,
    AActor* OwnerActor)
{
    if (!World)
        return false;

    // Create particle sphere shape
    FShape ParticleSphere;
    ParticleSphere.Kind = EShapeKind::Sphere;
    ParticleSphere.Sphere.SphereRadius = Radius;

    FTransform ParticleTransform(NewLocation, FQuat(0, 0, 0, 1), FVector::One());

    // Get all actors with shape components from world
    const TArray<AActor*>& AllActors = World->GetActors();

    for (AActor* Actor : AllActors)
    {
        if (!Actor)
            continue;

        // Skip self-collision with owner actor
        if (Actor == OwnerActor)
            continue;

        // Get shape components from actor
        const TArray<USceneComponent*>& SceneComponents = Actor->GetSceneComponents();
        for (USceneComponent* Comp : SceneComponents)
        {
            UShapeComponent* ShapeComp = Cast<UShapeComponent>(Comp);
            if (!ShapeComp)
                continue;

            // Skip if not generating overlap events
            if (!ShapeComp->GetGenerateOverlapEvents())
                continue;

            // Get the shape from the component
            FShape OtherShape;
            ShapeComp->GetShape(OtherShape);

            FTransform OtherTransform = ShapeComp->GetWorldTransform();

            // Check overlap using the collision system
            bool bOverlap = false;

            // Use appropriate overlap function based on shape types
            if (OtherShape.Kind == EShapeKind::Sphere)
            {
                bOverlap = Collision::OverlapSphereAndSphere(ParticleSphere, ParticleTransform, OtherShape, OtherTransform);
            }
            else if (OtherShape.Kind == EShapeKind::Capsule)
            {
                bOverlap = Collision::OverlapCapsuleAndSphere(OtherShape, OtherTransform, ParticleSphere, ParticleTransform);
            }
            else if (OtherShape.Kind == EShapeKind::Box)
            {
                // For box, we need to use OBB overlap
                FOBB BoxOBB;
                Collision::BuildOBB(OtherShape, OtherTransform, BoxOBB);
                bOverlap = Collision::Overlap_Sphere_OBB(NewLocation, Radius, BoxOBB);
            }

            if (bOverlap)
            {
                // Calculate hit location and normal
                FVector ToParticle = NewLocation - OtherTransform.Translation;
                float Distance = ToParticle.Size();

                if (Distance > 0.001f)
                {
                    OutHitNormal = ToParticle / Distance;
                }
                else
                {
                    OutHitNormal = FVector(0.0f, 0.0f, 1.0f);
                }

                // Calculate hit location on surface
                if (OtherShape.Kind == EShapeKind::Sphere)
                {
                    float OtherRadius = OtherShape.Sphere.SphereRadius * OtherTransform.Scale3D.X;
                    OutHitLocation = OtherTransform.Translation + OutHitNormal * OtherRadius;
                }
                else
                {
                    OutHitLocation = NewLocation - OutHitNormal * Radius;
                }

                OutHitActor = Actor;
                OutHitComponent = ShapeComp;

                return true;
            }
        }
    }

    return false;
}

void UParticleModuleCollision::ApplyCollisionResponse(
    FBaseParticle* Particle,
    const FVector& HitLocation,
    const FVector& HitNormal)
{
    switch (CollisionResponse)
    {
    case EParticleCollisionResponse::Bounce:
    {
        // Reflect velocity off the surface normal
        // v' = v - 2(v·n)n
        float VdotN = FVector::Dot(Particle->Velocity, HitNormal);
        FVector ReflectedVelocity = Particle->Velocity - HitNormal * (2.0f * VdotN);

        // Apply restitution (bounciness)
        ReflectedVelocity = ReflectedVelocity * Restitution;

        // Apply friction to tangential component
        FVector NormalComponent = HitNormal * VdotN;
        FVector TangentComponent = Particle->Velocity - NormalComponent;
        TangentComponent = TangentComponent * (1.0f - Friction);

        Particle->Velocity = ReflectedVelocity;

        // Apply damping
        Particle->Velocity = Particle->Velocity * (1.0f - DampingFactor);

        // Apply rotation damping
        Particle->RotationRate *= (1.0f - DampingFactorRotation);

        // Move particle to hit location (slightly offset to prevent re-collision)
        Particle->Location = HitLocation + HitNormal * 0.1f;
        break;
    }

    case EParticleCollisionResponse::Stop:
    {
        // Stop all movement
        Particle->Velocity = FVector::Zero();
        Particle->RotationRate = 0.0f;
        Particle->Location = HitLocation + HitNormal * 0.1f;
        break;
    }

    case EParticleCollisionResponse::Kill:
    {
        // Particle will be killed - set lifetime to trigger death
        Particle->RelativeTime = Particle->LifeTime;
        break;
    }
    }
}
