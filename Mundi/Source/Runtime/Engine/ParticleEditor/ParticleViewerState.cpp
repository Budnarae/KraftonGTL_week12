#include "pch.h"
#include "ParticleViewerState.h"

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

