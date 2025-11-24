#include "pch.h"
#include "ParticleModule.h"
#include "ParticleData.h"

UParticleModule::UParticleModule(int32 InPayloadSize) :
    PayloadSize(InPayloadSize)
{}

void UParticleModule::Spawn(FParticleContext& Context, float EmitterTime) {}
void UParticleModule::Update(FParticleContext& Context, float DeltaTime) {}

int32 UParticleModule::GetRequiredPayloadSize() const { return PayloadSize; }

int32 UParticleModule::GetPayloadOffset() const
{
    return PayloadOffset;
}
void UParticleModule::SetPayloadOffset(int32 NewOffset)
{
    PayloadOffset = NewOffset;
}
void UParticleModule::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
}