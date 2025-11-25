#pragma once
#include "Source/Runtime/Engine/Particle/ParticleSystem.h"

class UWorld; class FViewport; class FViewportClient;

class ParticleViewerState
{
public:
	ParticleViewerState() = default;
	~ParticleViewerState();
	UWorld* World = nullptr;
	FViewport* Viewport = nullptr;
	FViewportClient* Client = nullptr;

	FString ParticlePath;
	UParticleSystem* CachedParticle = nullptr;
	UParticleEmitter* SelectedEmitter = nullptr;
	UParticleModule* SelectedModule = nullptr;
	

	const char* GetPathConstChar() { return ParticlePath.empty() ? "None" : ParticlePath.c_str(); }
	void LoadCachedParticle(const FString& Path);
	void SaveCachedParticle();
};