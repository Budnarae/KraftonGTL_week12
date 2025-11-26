#include "pch.h"
#include "ParticleViewerState.h"
#include "Source/Runtime/Engine/GameFramework/ParticleSystemActor.h"
#include "Source/Runtime/Engine/Particle/ParticleSystemComponent.h"
#include "Source/Runtime/Engine/Particle/ParticleVariable.h"

ParticleViewerState::~ParticleViewerState()
{
	if (CachedParticle)
	{
		DeleteObject(CachedParticle);
		CachedParticle = nullptr;
	}
}
void ParticleViewerState::LoadCachedParticle(const FString& Path)
{
	if (CachedParticle)
	{
		DeleteObject(CachedParticle);
		CachedParticle = nullptr;
	}
	CachedParticle = NewObject<UParticleSystem>();
	UParticleAsset* ParticleAsset = UResourceManager::GetInstance().Load<UParticleAsset>(Path);
	ParticleAsset->GetDeepDuplicated(CachedParticle);
	ParticlePath = ParticleAsset->GetFilePath();

	UParticleSystemComponent* Component = ParticleActor->GetParticleSystemComponent();
	Component->SetTemplate(CachedParticle);

	// ParticleEditorWorld에서는 거리에 상관없이 선택된 LOD를 보여줌
	// 로드 직후에도 bOverrideLOD를 활성화하여 자동 LOD 전환을 막음
	Component->SetOverrideLOD(true);
	Component->SetCurrentLODLevel(SelectedLODLevel);

	ReStartParticle();
	SelectedEmitter = nullptr;
	SelectedModule = nullptr;
}
void ParticleViewerState::SaveCachedParticle()
{
	if (CachedParticle)
	{
		DeleteObject(CachedParticle);
		CachedParticle = nullptr;
	}
}
void ParticleViewerState::ReStartParticle()
{
	if (ParticleActor)
	{
		ParticleActor->GetParticleSystemComponent()->Deactivate();
		ParticleActor->GetParticleSystemComponent()->Activate(true);
	}
}

void ParticleViewerState::SetSelectedLODLevel(int32 InLODLevel)
{
	if (InLODLevel < 0 || InLODLevel >= MAX_PARTICLE_LODLEVEL)
	{
		return;
	}
	SelectedLODLevel = InLODLevel;

	// 모듈 선택 해제 (다른 LOD의 모듈일 수 있으므로)
	SelectedModule = nullptr;

	// ParticleEditorWorld에서는 거리에 상관없이 선택된 LOD를 보여줌
	// bOverrideLOD를 활성화하여 자동 LOD 전환을 막고, 직접 LOD 설정
	if (ParticleActor)
	{
		UParticleSystemComponent* Component = ParticleActor->GetParticleSystemComponent();
		if (Component)
		{
			Component->SetOverrideLOD(true);
			Component->SetCurrentLODLevel(InLODLevel);
		}
	}
}

UParticleLODLevel* ParticleViewerState::GetSelectedLODLevelInstance() const
{
	if (!SelectedEmitter)
	{
		return nullptr;
	}
	return SelectedEmitter->GetParticleLODLevelWithIndex(SelectedLODLevel);
}


