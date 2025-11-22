#include "pch.h"
#include "ParticleSystemComponent.h"

// 시뮬레이션 시작 명령 (파티클 생성 및 타이머 시작)
void UParticleSystemComponent::Activate(bool bReset)
{
    ;
}

// 시뮬레이션 종료 명령 (새 파티클 생성을 중단하고 기존 파티클이 수명을 다하도록 둠)
void UParticleSystemComponent::Deactivate()
{
    ;
}

// [Tick Phase] 매 프레임 호출되어 DeltaTime만큼 시뮬레이션을 전진시킵니다. (가장 중요)
void UParticleSystemComponent::TickComponent(float DeltaTime)
{
    ;
}
    
// 모든 파티클을 즉시 중지하고 메모리를 정리합니다. (강제 종료)
void UParticleSystemComponent::KillParticlesAndCleanUp()
{
    ;
}
    
// 이 컴포넌트가 현재 재생 중인 이펙트를 멈추고 메모리를 해제합니다. (소멸자 등에서 호출)
void UParticleSystemComponent::BeginDestroy()
{
    ;
}