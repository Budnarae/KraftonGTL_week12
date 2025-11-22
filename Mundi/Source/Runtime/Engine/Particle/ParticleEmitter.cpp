#include "pch.h"
#include "ParticleEmitter.h"

#include "ParticleData.h"
#include "ParticleLODLevel.h"

// [셋업 단계] 모든 모듈을 순회하며 파티클 크기(ParticleSize) 및 페이로드 오프셋을 계산하고 캐시합니다.
void UParticleEmitter::CacheEmitterModuleInfo()
{
    if (CurrentLODLevel <= INVALID) return;
    if (CurrentLODLevel >= LODLevels.Num()) return;

    UParticleLODLevel* LODLevel = LODLevels[CurrentLODLevel];

    int32 AccumulatedPayload = 0;
    
    for (UParticleModule* ParticleModule : LODLevel->GetModule())
    {
        int32 PayloadSize = ParticleModule->GetRequiredPayloadSize();
        if (PayloadSize == 0)
            ParticleModule->SetPayloadOffset(0);
        else
        {
            ParticleModule->SetPayloadOffset(AccumulatedPayload);
            AccumulatedPayload += PayloadSize;
        }
    }

    ParticleSize = sizeof(FBaseParticle) + AccumulatedPayload;
    // ParticleSize는 16 byte의 배수여야 하므로 padding을 부여한다.
    ParticleSize += ParticleSize % 16;
}

// 이 에미터의 파티클 시뮬레이션 및 렌더링에 필요한 총 메모리 크기를 반환합니다.
int64 UParticleEmitter::GetRequiredMemorySize() const
{
    return ParticleSize;
}

uint32 UParticleEmitter::GetCurrentLODLevel()
{
    return CurrentLODLevel;
}

void UParticleEmitter::SetCurrentLODLevel(const uint32 InCurrentLODLevel)
{
    if (InCurrentLODLevel > LODLevels.Num())
    {
        UE_LOG("[UParticleEmitter::SetCurrentLODLevel][Warninig] There's no such LODLevel.");
        return;
    }
    
    CurrentLODLevel = InCurrentLODLevel;
}

// 에디터에서 새 LOD 레벨을 추가합니다.
void UParticleEmitter::AddLODLevel()
{
    LODLevels.Add(NewObject<UParticleLODLevel>());
}

// 참조로 LOD 레벨을 제거한다.
bool UParticleEmitter::RemoveLODLevel(UParticleLODLevel* Target)
{
    bool result = LODLevels.Remove(Target);
    if (result && LODLevels.Num() <= CurrentLODLevel)
    {
        // 삭제로 LODLevel이 유효성을 잃으면 강제로 재지정한다.
        CurrentLODLevel = LODLevels.Num() - 1;
    }

    return result;
}
    
// index로 LOD 레벨을 제거한다.
void UParticleEmitter::RemoveLODLevel(uint32 index)
{
    return LODLevels.RemoveAt(index);
}

int32 UParticleEmitter::GetMaxParticleCount()
{
    return MaxParticleCount;
}

void UParticleEmitter::SetMaxParticleCount(const int32 InMaxParticleCount)
{
    MaxParticleCount = InMaxParticleCount;
}

float UParticleEmitter::GetCalculatedDuration()
{
    if (!IsValid()) return 0.f;
    if (CurrentLODLevel < 0 || CurrentLODLevel > LODLevels.Num()) return 0.f;
    UParticleModuleRequired* ModuleRequired =
        LODLevels[CurrentLODLevel]->GetRequiredModule();
    if (!ModuleRequired) return 0.f;
        
    return LODLevels[CurrentLODLevel]->GetRequiredModule()->GetEmitterDuration();
}

bool UParticleEmitter::IsValid() const
{
    for (UParticleLODLevel* LODLevel : LODLevels)
    {
        if (!LODLevel->IsValid())
            return false;
    }
    return true;
}
void UParticleEmitter::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
}