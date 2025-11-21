#include "pch.h"
#include "ParticleEmitter.h"

// // [셋업 단계] 모든 모듈을 순회하며 파티클 크기(ParticleSize) 및 페이로드 오프셋을 계산하고 캐시합니다.
// void UParticleEmitter::CacheEmitterModuleInfo()
// {
//     ;
// }
//
// // 이 에미터의 파티클 시뮬레이션 및 렌더링에 필요한 총 메모리 크기를 반환합니다.
// int64 UParticleEmitter::GetRequiredMemorySize() const
// {
//     return ParticleSize;
// }

// 에디터에서 새 LOD 레벨을 추가합니다.
void UParticleEmitter::AddLODLevel()
{
    LODLevels.Add(NewObject<UParticleLODLevel>());
}

// 참조로 LOD 레벨을 제거한다.
bool UParticleEmitter::RemoveLODLevel(UParticleLODLevel* Target)
{
    return LODLevels.Remove(Target);
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