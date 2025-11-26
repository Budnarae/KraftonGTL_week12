#include "pch.h"
#include "ParticleViewerState.h"
#include "Source/Runtime/Engine/GameFramework/ParticleSystemActor.h"
#include "Source/Runtime/Engine/Particle/ParticleSystemComponent.h"

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
	ParticleActor->GetParticleSystemComponent()->SetTemplate(CachedParticle);
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
void ParticleViewerState::SetLOD(int MoveOffset)
{
	if (ParticleActor)
	{
		ParticleActor->GetParticleSystemComponent()->SetCurrentLODLevel(ParticleActor->GetParticleSystemComponent()->GetCurrentLODLevel() + MoveOffset);
	}
}


