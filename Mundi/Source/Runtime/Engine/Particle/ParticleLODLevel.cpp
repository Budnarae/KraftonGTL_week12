#include "pch.h"
#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"

#include "Keyboard.h"

// 모듈 배열에 새 모듈을 추가 (에디터 기능에 필수)
void UParticleLODLevel::AddModule(UParticleModule* NewModule)
{
    if (!NewModule)
    {
        UE_LOG("[UParticleLODLevel::AddModule][Warning] Parameter is null.");
        return;
    }

    Modules.Add(NewModule);
}
    
// 모듈 배열에서 특정 모듈을 제거
bool UParticleLODLevel::RemoveModule(UParticleModule* TargetModule)
{
    return Modules.Remove(TargetModule);
}

UParticleModuleRequired* UParticleLODLevel::GetRequiredModule()
{
    return RequiredModule;
}

void UParticleLODLevel::CreateRequiredModule()
{
    if (RequiredModule)
    {
        UE_LOG("[UParticleLODLevel::CreateRequiredModule][Warning]There's already Required Module.");
        return;
    }
    RequiredModule = NewObject<UParticleModuleRequired>();
}

void UParticleLODLevel::DeleteRequiredModule()
{
    if (!RequiredModule)
    {
        UE_LOG("[UParticleLODLevel::DeleteRequiredModule][Warning]Required Module is already null.");
        return;
    }
    DeleteObject(RequiredModule);
}

TArray<UParticleModule*>& UParticleLODLevel::GetModule()
{
    return Modules;
}
    
// 이 LOD 레벨의 설정이 유효한지 확인
bool UParticleLODLevel::IsValid() const
{
    if (!RequiredModule) return false;
    
    return true;
}
void UParticleLODLevel::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
}