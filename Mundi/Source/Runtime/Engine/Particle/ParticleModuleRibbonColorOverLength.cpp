#include "pch.h"
#include "ParticleModuleRibbonColorOverLength.h"

UParticleModuleRibbonColorOverLength::UParticleModuleRibbonColorOverLength()
{
    // Default: white (opaque) at head, white (transparent) at tail
    // This creates a natural fade-out effect for trails
    HeadColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    TailColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);
}

void UParticleModuleRibbonColorOverLength::Spawn(FParticleContext& Context, float EmitterTime)
{
    // Ribbon color is applied at render time, not per-particle spawn
    // This module doesn't need per-particle spawn logic
}

void UParticleModuleRibbonColorOverLength::Update(FParticleContext& Context, float DeltaTime)
{
    // Ribbon color is applied at render time, not per-particle update
    // This module doesn't need per-particle update logic
}
