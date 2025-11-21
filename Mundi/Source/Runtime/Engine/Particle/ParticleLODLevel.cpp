#include "pch.h"
#include "ParticleLODLevel.h"

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
    
// // 이 LOD 레벨의 설정이 유효한지 확인 (예: RequiredModule이 null인지 체크)
// bool UParticleLODLevel::IsValidConfiguration() const
// {
//     if (!RequiredModule) return false;
//     
//     return true;
// }