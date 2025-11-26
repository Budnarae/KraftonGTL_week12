#include "pch.h"
#include "ParticleSystem.h"

UParticleSystem::UParticleSystem()
{
}

// 새 Emitter를 시스템에 추가합니다. (에디터 기능)
void UParticleSystem::AddEmitter(UParticleEmitter* NewEmitter)
{
    if (!NewEmitter)
    {
        UE_LOG("[UParticleSystem::AddEmitter][Warning]Parameter is null.");
        return;
    }

    Emitters.Add(NewEmitter);
}

// Emitter 리스트에서 특정 Emitter를 제거합니다.
bool UParticleSystem::RemoveEmitter(UParticleEmitter* TargetEmitter)
{
    return Emitters.Remove(TargetEmitter);
}

TArray<UParticleEmitter*>& UParticleSystem::GetEmitters()
{
    return Emitters;
}

// 시스템의 전체 Duration을 Emitters의 설정에 기반하여 계산합니다.
float UParticleSystem::GetCalculatedDuration() const
{
    float MaxDuration = 0.f;

    for (UParticleEmitter* Emitter : Emitters)
    {
        MaxDuration = std::max(Emitter->GetCalculatedDuration(), MaxDuration);
    }

    return MaxDuration;
}

// 시스템 전체의 상태가 유효한지 검증합니다.
bool UParticleSystem::IsValid() const
{
    for (UParticleEmitter* Emitter : Emitters)
    {
        if (!Emitter->IsValid())
        {
            return false;
        }
    }
    return true;
}
void UParticleSystem::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
}

void UParticleSystem::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
}

// -------------------------------------------
// LOD 관련 함수 구현
// -------------------------------------------

int32 UParticleSystem::GetLODLevelForDistance(float Distance) const
{
    // Emitter가 없으면 항상 LOD 0
    if (Emitters.Num() == 0)
    {
        return 0;
    }

    // 거리가 음수면 LOD 0
    if (Distance < 0.0f)
    {
        return 0;
    }

    // 첫 번째 Emitter의 LODLevels를 기준으로 판단
    // (모든 Emitter가 동일한 LOD 거리 설정을 공유한다고 가정)
    UParticleEmitter* FirstEmitter = Emitters[0];
    if (!FirstEmitter)
    {
        return 0;
    }

    // LOD 0은 항상 기본값 (거리 0.0)
    int32 ResultLOD = 0;

    // 정순으로 순회하여 Distance 이상인 가장 높은 LOD 찾기
    // 예: LOD 0 = 0.0, LOD 1 = 1000.0, LOD 2 = 2000.0
    //     Distance = 1500 → LOD 1 반환 (1000 <= 1500 < 2000)
    //
    // LOD 1+ 는 LODDistance가 0보다 커야 유효한 것으로 간주
    // (LODDistance가 0.0이면 해당 LOD는 사용하지 않음)
    for (int32 i = 1; i < MAX_PARTICLE_LODLEVEL; ++i)
    {
        UParticleLODLevel* LODLevel = FirstEmitter->GetParticleLODLevelWithIndex(i);
        if (!LODLevel) continue;

        float LODDist = LODLevel->GetLODDistance();

        // LODDistance가 0.0 이하면 유효하지 않은 LOD (설정되지 않음)
        if (LODDist <= 0.0f) continue;

        // Distance가 이 LOD의 거리 이상이면 이 LOD가 후보
        if (Distance >= LODDist)
        {
            ResultLOD = i;
        }
        else
        {
            // Distance가 이 LOD의 거리보다 작으면 더 이상 체크할 필요 없음
            break;
        }
    }

    return ResultLOD;
}
