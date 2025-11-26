#include "pch.h"
#include "ParticleSystem.h"

UParticleSystem::UParticleSystem()
{
    // 기본 에미터 추가
    AddEmitter(NewObject<UParticleEmitter>());
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