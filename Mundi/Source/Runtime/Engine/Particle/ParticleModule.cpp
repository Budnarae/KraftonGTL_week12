#include "pch.h"
#include "ParticleModule.h"

void UParticleModule::Spawn(FBaseParticle* Particle, float EmitterTime) {}
void UParticleModule::Update(FBaseParticle* Particle, float DeltaTime) {}
int32 UParticleModule::GetRequiredPayloadSize() const { return 0; }

int32 UParticleModule::GetPayloadOffset() const
{
    return MyPayloadOffset;
}
void UParticleModule::SetPayloadOffset(int32 NewOffset)
{
    MyPayloadOffset = NewOffset;
}