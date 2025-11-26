#include "pch.h"
#include "ParticleModuleRibbonWidth.h"
#include "ParticleData.h"

UParticleModuleRibbonWidth::UParticleModuleRibbonWidth()
    : UParticleModule()
{
}

void UParticleModuleRibbonWidth::Spawn(FParticleContext& Context, float EmitterTime)
{
    // Ribbon width is evaluated per-particle during rendering
    // No spawn-time initialization needed
}

void UParticleModuleRibbonWidth::Update(FParticleContext& Context, float DeltaTime)
{
    // Width parameters are applied during ribbon mesh generation
    // No per-frame update needed for this module
}
