#include "pch.h"
#include "ParticleData.h"

#include "ParticleEmitter.h"
#include "ParticleModuleRequired.h"
#include "ParticleHelper.h"

void FParticleEmitterInstance::Update(float DeltaTime)
{
    for (int32 Index = ActiveParticles - 1; Index >= 0; Index--)
    {
        DECLARE_PARTICLE_PTR(ParticleBase, ParticleData + ParticleStride * Index);
        UParticleModuleRequired* RequiredModule = \
            SpriteTemplate->GetCurrentLODLevelInstance()->GetRequiredModule();

        RequiredModule->Update(ParticleBase, DeltaTime);

        TArray<UParticleModule*>& UpdateModules = \
            SpriteTemplate->GetCurrentLODLevelInstance()->GetUpdateModule();

        for (UParticleModule* UpdateModule : UpdateModules)
        {
            UpdateModule->Update(ParticleBase, DeltaTime);
        }

        if (ParticleBase->RelativeTime >= ParticleBase->LifeTime)
            KillParticle(Index);
    }
    
    float SpawnNumFraction = SpawnRate * DeltaTime + SpawnFraction;
    SpawnNum = floor(SpawnNumFraction);
    SpawnFraction = SpawnNumFraction - SpawnNum;
}

void FParticleEmitterInstance::SpawnParticles
(
    float StartTime,
    float Increment,
    const FVector& InitialLocation,
    const FVector& InitialVelocity,
    FParticleEventInstancePayload* EventPayload
)
{
    for (int32 Index = 0; Index < SpawnNum; Index++)
    {
        if (ActiveParticles >= MaxActiveParticles)
        {
            UE_LOG("[FParticleEmitterInstance::SpawnParticles][Warning] Reached max particle count.");
            break;
        }

        DECLARE_PARTICLE_PTR(ParticlePtr, ParticleData + ParticleStride * ActiveParticles);
        ActiveParticles++;
        
        FBaseParticle& ParticleBase = *((FBaseParticle*)ParticlePtr);

        float LifeTime = GetLifeTimeValue();
        ParticleBase.LifeTime = LifeTime;
        ParticleBase.OneOverMaxLifetime = (LifeTime > 0.0f) ? (1.0f / LifeTime) : 0.0f; // 역수 설정

        // 파티클의 RelativeTime 설정 (Increment를 이용해 시간 분산)
        ParticleBase.RelativeTime = (float)Index * Increment;

        ParticleBase.Location = InitialLocation;
        ParticleBase.OldLocation = InitialLocation; // OldLocation도 초기 위치와 동일하게 설정
        
        ParticleBase.Velocity = InitialVelocity;
        ParticleBase.BaseVelocity = InitialVelocity; // BaseVelocity는 초기 속도 참조용

        // RequiredModule의 Spawn 호출 (필수)
        UParticleModuleRequired* RequiredModule = SpriteTemplate->GetCurrentLODLevelInstance()->GetRequiredModule();
        if (RequiredModule)
        {
            RequiredModule->Spawn(&ParticleBase, StartTime);
        }

        // 추가 SpawnModules 호출
        for (UParticleModule* Module : SpriteTemplate->GetCurrentLODLevelInstance()->GetSpawnModule())
        {
            Module->Spawn(&ParticleBase, StartTime);
        }
    }
}

void FParticleEmitterInstance::KillParticle(int32 Index)
{
    if (Index >= ActiveParticles || Index < 0)
    {
        UE_LOG("[FParticleEmitterInstance::KillParticle][Warning] Invalid Index.");
        return;
    }

    ActiveParticles--;

    // 마지막 요소가 아니면 swap-and-pop
    if (Index != ActiveParticles)
    {
        DECLARE_PARTICLE_PTR(Target, ParticleData + ParticleStride * Index);
        DECLARE_PARTICLE_PTR(Last, ParticleData + ParticleStride * ActiveParticles);
        memcpy(Target, Last, ParticleStride);
    }
}