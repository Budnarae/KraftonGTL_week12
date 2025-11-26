#include "pch.h"
#include "ParticleModuleBeamNoise.h"

UParticleModuleBeamNoise::UParticleModuleBeamNoise()
    : UParticleModule()
{
}

void UParticleModuleBeamNoise::Spawn(FParticleContext& Context, float EmitterTime)
{
    // Beam noise is applied at render time, not per-particle spawn
}

void UParticleModuleBeamNoise::Update(FParticleContext& Context, float DeltaTime)
{
    // Beam noise is applied at render time, not per-particle update
}
