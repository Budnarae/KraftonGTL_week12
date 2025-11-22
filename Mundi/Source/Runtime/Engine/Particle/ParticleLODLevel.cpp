#include "pch.h"
#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"

#include "Keyboard.h"

UParticleLODLevel::UParticleLODLevel()
{
    // CreateRequiredModule();
}

// SpawnModule 관련 함수
void UParticleLODLevel::AddSpawnModule(UParticleModule* NewModule)
{
    if (!NewModule)
    {
        UE_LOG("[UParticleLODLevel::AddSpawnModule][Warning] Parameter is null.");
        return;
    }

    SpawnModules.Add(NewModule);
}

bool UParticleLODLevel::RemoveSpawnModule(UParticleModule* TargetModule)
{
    return SpawnModules.Remove(TargetModule);
}

TArray<UParticleModule*>& UParticleLODLevel::GetSpawnModule()
{
    return SpawnModules;
}

// UpdateModule 관련 함수
void UParticleLODLevel::AddUpdateModule(UParticleModule* NewModule)
{
    if (!NewModule)
    {
        UE_LOG("[UParticleLODLevel::AddModule][Warning] Parameter is null.");
        return;
    }

    UpdateModules.Add(NewModule);
}
    
// 모듈 배열에서 특정 모듈을 제거
bool UParticleLODLevel::RemoveUpdateModule(UParticleModule* TargetModule)
{
    return UpdateModules.Remove(TargetModule);
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

TArray<UParticleModule*>& UParticleLODLevel::GetUpdateModule()
{
    return UpdateModules;
}
    
// 이 LOD 레벨의 설정이 유효한지 확인
bool UParticleLODLevel::IsValid() const
{
    if (!RequiredModule) return false;
    
    return true;
}