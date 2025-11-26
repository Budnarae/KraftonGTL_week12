#include "pch.h"
#include "ParticleModuleBeamColorOverLength.h"

UParticleModuleBeamColorOverLength::UParticleModuleBeamColorOverLength()
{
    // Default: white to white (no gradient)
    StartColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    EndColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    bEnabled = true;
}

void UParticleModuleBeamColorOverLength::Spawn(FParticleContext& Context, float EmitterTime)
{
    // Beam color is applied at render time, not per-particle spawn
    // This module doesn't need per-particle spawn logic
}

void UParticleModuleBeamColorOverLength::Update(FParticleContext& Context, float DeltaTime)
{
    // Beam color is applied at render time, not per-particle update
    // This module doesn't need per-particle update logic
}
