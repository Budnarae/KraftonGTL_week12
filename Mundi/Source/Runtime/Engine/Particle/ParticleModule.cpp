#include "pch.h"
#include "ParticleModule.h"
#include "ParticleData.h"

UParticleModule::UParticleModule()
{
    // 기본값: 모든 LOD에서 활성화
    EnableInAllLODs();
}

UParticleModule::UParticleModule(int32 InPayloadSize) :
    PayloadSize(InPayloadSize)
{
    // 기본값: 모든 LOD에서 활성화
    EnableInAllLODs();
}

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

// -------------------------------------------
// LOD별 활성화 함수 구현
// -------------------------------------------

bool UParticleModule::IsEnabledInLOD(int32 LODIndex) const
{
    if (LODIndex < MIN_PARTICLE_LODLEVEL || LODIndex >= MAX_PARTICLE_LODLEVEL)
    {
        return false;
    }
    return bEnabledInLOD[LODIndex];
}

void UParticleModule::SetEnabledInLOD(int32 LODIndex, bool bEnabled)
{
    if (LODIndex < MIN_PARTICLE_LODLEVEL || LODIndex >= MAX_PARTICLE_LODLEVEL)
    {
        UE_LOG("[UParticleModule::SetEnabledInLOD][Warning] Invalid LOD index: %d", LODIndex);
        return;
    }
    bEnabledInLOD[LODIndex] = bEnabled;
}

void UParticleModule::EnableInAllLODs()
{
    for (int32 i = 0; i < MAX_PARTICLE_LODLEVEL; ++i)
    {
        bEnabledInLOD[i] = true;
    }
}

void UParticleModule::EnableBelowLOD(int32 MaxLOD)
{
    for (int32 i = 0; i < MAX_PARTICLE_LODLEVEL; ++i)
    {
        bEnabledInLOD[i] = (i < MaxLOD);
    }
}

bool UParticleModule::ShouldExecuteInLOD(int32 LODIndex) const
{
    // 전체 비활성화 상태면 모든 LOD에서 실행 안 함
    if (!bActive)
    {
        return false;
    }

    // LOD 범위 체크
    if (LODIndex < MIN_PARTICLE_LODLEVEL || LODIndex >= MAX_PARTICLE_LODLEVEL)
    {
        return false;
    }

    // LOD별 활성화 상태 확인
    return bEnabledInLOD[LODIndex];
}

void UParticleModule::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
}