#include "pch.h"
#include "ParticleModuleTypeDataBase.h"
#include "ParticleData.h"

UParticleModuleTypeDataBase::UParticleModuleTypeDataBase()
    : UParticleModule(0) // No payload needed for base type
{
}

void UParticleModuleTypeDataBase::Spawn(FParticleContext& Context, float EmitterTime)
{
    // Base implementation - can be overridden by derived classes
}

void UParticleModuleTypeDataBase::Update(FParticleContext& Context, float DeltaTime)
{
    // Base implementation - can be overridden by derived classes
}
