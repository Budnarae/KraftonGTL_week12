#include "pch.h"
#include "ParticleModuleBeamWidth.h"

UParticleModuleBeamWidth::UParticleModuleBeamWidth()
    : UParticleModule()
{
}

void UParticleModuleBeamWidth::Spawn(FParticleContext& Context, float EmitterTime)
{
    // Beam width is resolved at render time, not per-particle spawn
}

void UParticleModuleBeamWidth::Update(FParticleContext& Context, float DeltaTime)
{
    // Beam width is resolved at render time, not per-particle update
}
