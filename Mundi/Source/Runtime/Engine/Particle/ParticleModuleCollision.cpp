#include "pch.h"
#include "ParticleModuleCollision.h"
#include "ParticleSystemComponent.h"
#include "ParticleEventTypes.h"
#include "World.h"
#include "Actor.h"
#include "ShapeComponent.h"
#include "Collision.h"
#include "OBB.h"
#include "AABB.h"

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

    // ========================================
    // 1. 파티클 이동 경로를 감싸는 AABB 생성
    // ========================================
    FAABB ParticleAABB;
    ParticleAABB.Min = FVector(
        FMath::Min(OldLocation.X, NewLocation.X) - Radius,
        FMath::Min(OldLocation.Y, NewLocation.Y) - Radius,
        FMath::Min(OldLocation.Z, NewLocation.Z) - Radius
    );
    ParticleAABB.Max = FVector(
        FMath::Max(OldLocation.X, NewLocation.X) + Radius,
        FMath::Max(OldLocation.Y, NewLocation.Y) + Radius,
        FMath::Max(OldLocation.Z, NewLocation.Z) + Radius
    );

    // 파티클 Shape 생성 (Sphere)
    FShape ParticleSphere;
    ParticleSphere.Kind = EShapeKind::Sphere;
    ParticleSphere.Sphere.SphereRadius = Radius;

    // 샘플링 설정 (이동 거리에 따라 적응형)
    float MoveDistance = (NewLocation - OldLocation).Size();
    int NumSamples = FMath::Max(4, static_cast<int>(MoveDistance / FMath::Max(Radius, 1.0f)) + 1);
    NumSamples = FMath::Min(NumSamples, 16);

    // Track closest hit
    float ClosestHitT = FLT_MAX;
    bool bFoundHit = false;

    const TArray<AActor*>& AllActors = World->GetActors();

    for (AActor* Actor : AllActors)
    {
        if (!Actor || Actor == OwnerActor)
            continue;

        const TArray<USceneComponent*>& SceneComponents = Actor->GetSceneComponents();
        for (USceneComponent* Comp : SceneComponents)
        {
            UShapeComponent* ShapeComp = Cast<UShapeComponent>(Comp);
            if (!ShapeComp || !ShapeComp->GetGenerateOverlapEvents())
                continue;

            // ========================================
            // 2. Broad Phase: Shape의 실제 AABB 계산 및 교차 검사
            // ========================================
            FShape OtherShape;
            ShapeComp->GetShape(OtherShape);
            FTransform OtherTransform = ShapeComp->GetWorldTransform();

            // Shape의 실제 월드 AABB 계산
            FAABB ShapeAABB;
            FVector ShapeCenter = OtherTransform.Translation;

            if (OtherShape.Kind == EShapeKind::Sphere)
            {
                float WorldRadius = OtherShape.Sphere.SphereRadius *
                    FMath::Max(FMath::Abs(OtherTransform.Scale3D.X),
                    FMath::Max(FMath::Abs(OtherTransform.Scale3D.Y),
                               FMath::Abs(OtherTransform.Scale3D.Z)));
                ShapeAABB.Min = ShapeCenter - FVector(WorldRadius, WorldRadius, WorldRadius);
                ShapeAABB.Max = ShapeCenter + FVector(WorldRadius, WorldRadius, WorldRadius);
            }
            else if (OtherShape.Kind == EShapeKind::Box)
            {
                // Box의 경우 회전을 고려한 AABB 계산
                FVector HalfExtent = OtherShape.Box.BoxExtent;
                HalfExtent.X *= FMath::Abs(OtherTransform.Scale3D.X);
                HalfExtent.Y *= FMath::Abs(OtherTransform.Scale3D.Y);
                HalfExtent.Z *= FMath::Abs(OtherTransform.Scale3D.Z);

                // 회전을 고려한 최대 extent 계산
                FMatrix RotMat = OtherTransform.Rotation.ToMatrix();
                FVector WorldExtent(
                    FMath::Abs(RotMat.M[0][0]) * HalfExtent.X + FMath::Abs(RotMat.M[1][0]) * HalfExtent.Y + FMath::Abs(RotMat.M[2][0]) * HalfExtent.Z,
                    FMath::Abs(RotMat.M[0][1]) * HalfExtent.X + FMath::Abs(RotMat.M[1][1]) * HalfExtent.Y + FMath::Abs(RotMat.M[2][1]) * HalfExtent.Z,
                    FMath::Abs(RotMat.M[0][2]) * HalfExtent.X + FMath::Abs(RotMat.M[1][2]) * HalfExtent.Y + FMath::Abs(RotMat.M[2][2]) * HalfExtent.Z
                );
                ShapeAABB.Min = ShapeCenter - WorldExtent;
                ShapeAABB.Max = ShapeCenter + WorldExtent;
            }
            else if (OtherShape.Kind == EShapeKind::Capsule)
            {
                float WorldRadius = OtherShape.Capsule.CapsuleRadius *
                    FMath::Max(FMath::Abs(OtherTransform.Scale3D.X), FMath::Abs(OtherTransform.Scale3D.Y));
                float WorldHalfHeight = OtherShape.Capsule.CapsuleHalfHeight * FMath::Abs(OtherTransform.Scale3D.Z);

                // 캡슐의 축 방향 계산
                FVector CapsuleUp = OtherTransform.Rotation.RotateVector(FVector(0, 0, 1));
                FVector AbsUp(FMath::Abs(CapsuleUp.X), FMath::Abs(CapsuleUp.Y), FMath::Abs(CapsuleUp.Z));

                FVector HalfExtent = AbsUp * WorldHalfHeight + FVector(WorldRadius, WorldRadius, WorldRadius);
                ShapeAABB.Min = ShapeCenter - HalfExtent;
                ShapeAABB.Max = ShapeCenter + HalfExtent;
            }

            if (!ParticleAABB.Intersects(ShapeAABB))
                continue;  // AABB가 안 겹치면 스킵

            // ========================================
            // 3. Narrow Phase: 기존 OverlapLUT 사용 (샘플링)
            // ========================================
            for (int i = 0; i <= NumSamples; ++i)
            {
                float t = static_cast<float>(i) / static_cast<float>(NumSamples);
                FVector SamplePos = OldLocation + (NewLocation - OldLocation) * t;
                FTransform SampleTransform(SamplePos, FQuat(0, 0, 0, 1), FVector::One());

                // 기존 OverlapLUT 사용! (Sphere = 1)
                bool bOverlap = Collision::OverlapLUT[1][static_cast<int>(OtherShape.Kind)](
                    ParticleSphere, SampleTransform, OtherShape, OtherTransform);

                if (bOverlap && t < ClosestHitT)
                {
                    ClosestHitT = t;
                    bFoundHit = true;

                    // ========================================
                    // 4. Hit Normal 계산
                    // ========================================
                    FVector HitLocation = SamplePos;

                    if (OtherShape.Kind == EShapeKind::Sphere)
                    {
                        FVector SphereCenter = OtherTransform.Translation;
                        float SphereRadius = OtherShape.Sphere.SphereRadius * OtherTransform.Scale3D.X;
                        FVector ToHit = HitLocation - SphereCenter;
                        float Dist = ToHit.Size();
                        OutHitNormal = (Dist > 0.001f) ? ToHit / Dist : FVector(0, 0, 1);
                        OutHitLocation = SphereCenter + OutHitNormal * SphereRadius;
                    }
                    else if (OtherShape.Kind == EShapeKind::Box)
                    {
                        FOBB BoxOBB;
                        Collision::BuildOBB(OtherShape, OtherTransform, BoxOBB);

                        FVector LocalPos = HitLocation - BoxOBB.Center;
                        FVector LocalPosInBox(
                            FVector::Dot(LocalPos, BoxOBB.Axes[0]),
                            FVector::Dot(LocalPos, BoxOBB.Axes[1]),
                            FVector::Dot(LocalPos, BoxOBB.Axes[2])
                        );

                        float HalfExt[3] = { BoxOBB.HalfExtent.X, BoxOBB.HalfExtent.Y, BoxOBB.HalfExtent.Z };
                        float MinPen = FLT_MAX;
                        int BestAxis = 0, BestSign = 1;

                        for (int j = 0; j < 3; ++j)
                        {
                            float PenPos = HalfExt[j] - LocalPosInBox[j];
                            float PenNeg = HalfExt[j] + LocalPosInBox[j];
                            if (PenPos < MinPen) { MinPen = PenPos; BestAxis = j; BestSign = 1; }
                            if (PenNeg < MinPen) { MinPen = PenNeg; BestAxis = j; BestSign = -1; }
                        }

                        OutHitNormal = BoxOBB.Axes[BestAxis] * static_cast<float>(BestSign);
                        FVector Clamped = LocalPosInBox;
                        Clamped[BestAxis] = HalfExt[BestAxis] * static_cast<float>(BestSign);
                        OutHitLocation = BoxOBB.Center + BoxOBB.Axes[0] * Clamped.X + BoxOBB.Axes[1] * Clamped.Y + BoxOBB.Axes[2] * Clamped.Z;
                    }
                    else if (OtherShape.Kind == EShapeKind::Capsule)
                    {
                        FVector CapsuleUp = OtherTransform.Rotation.RotateVector(FVector(0, 0, 1));
                        float HalfHeight = OtherShape.Capsule.CapsuleHalfHeight * OtherTransform.Scale3D.Z;
                        float CapRadius = OtherShape.Capsule.CapsuleRadius * OtherTransform.Scale3D.X;

                        FVector CapsuleCenter = OtherTransform.Translation;
                        float AxisProj = FMath::Clamp(FVector::Dot(HitLocation - CapsuleCenter, CapsuleUp), -HalfHeight, HalfHeight);
                        FVector ClosestOnAxis = CapsuleCenter + CapsuleUp * AxisProj;
                        FVector ToHitFromAxis = HitLocation - ClosestOnAxis;
                        float DistFromAxis = ToHitFromAxis.Size();

                        OutHitNormal = (DistFromAxis > 0.001f) ? ToHitFromAxis / DistFromAxis : FVector(0, 0, 1);
                        OutHitLocation = ClosestOnAxis + OutHitNormal * CapRadius;
                    }

                    OutHitActor = Actor;
                    OutHitComponent = ShapeComp;
                    break;  // 이 Shape에서 첫 충돌 발견
                }
            }
        }
    }

    return bFoundHit;
}

void UParticleModuleCollision::ApplyCollisionResponse(
    FBaseParticle* Particle,
    const FVector& HitLocation,
    const FVector& HitNormal)
{
    // Calculate effective radius for position offset
    float EffectiveRadius = CollisionRadius;
    if (EffectiveRadius <= 0.0f)
    {
        EffectiveRadius = Particle->Size.X * 0.5f;
    }

    // Offset to push particle outside collision area (radius + small margin)
    float PositionOffset = EffectiveRadius + 0.5f;

    switch (CollisionResponse)
    {
    case EParticleCollisionResponse::Bounce:
    {
        // Only reflect if particle is moving toward the surface
        float VdotN = FVector::Dot(Particle->Velocity, HitNormal);

        if (VdotN < 0.0f) // Moving toward surface
        {
            // Reflect velocity off the surface normal
            // v' = v - 2(v·n)n
            FVector ReflectedVelocity = Particle->Velocity - HitNormal * (2.0f * VdotN);

            // Apply restitution (bounciness)
            ReflectedVelocity = ReflectedVelocity * Restitution;

            // Apply friction to tangential component
            FVector NormalComponent = HitNormal * VdotN;
            FVector TangentComponent = Particle->Velocity - NormalComponent;
            ReflectedVelocity = ReflectedVelocity - TangentComponent * Friction;

            Particle->Velocity = ReflectedVelocity;

            // Apply damping
            Particle->Velocity = Particle->Velocity * (1.0f - DampingFactor);

            // Apply rotation damping
            Particle->RotationRate *= (1.0f - DampingFactorRotation);
        }

        // Move particle to surface + offset to prevent re-collision
        Particle->Location = HitLocation + HitNormal * PositionOffset;
        break;
    }

    case EParticleCollisionResponse::Stop:
    {
        // Stop all movement
        Particle->Velocity = FVector::Zero();
        Particle->RotationRate = 0.0f;
        Particle->Location = HitLocation + HitNormal * PositionOffset;
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
