#pragma once
#include "Source/Runtime/Engine/Particle/ParticleSystem.h"
#include "Source/Runtime/Engine/GameFramework/ParticleSystemActor.h"
#include "Source/Runtime/Engine/GameFramework/AmbientLightActor.h"
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
	UParticleEmitter* SelectedEmitter = nullptr;
	UParticleModule* SelectedModule = nullptr;

	const char* GetPathConstChar() { return ParticlePath.empty() ? "None" : ParticlePath.c_str(); }
	void LoadCachedParticle(const FString& Path);
	void SaveCachedParticle();
	void ReStartParticle();

	const UParticleSystem* GetCachedParticle()const { return CachedParticle; }
	UParticleSystem* GetCachedParticle() { return CachedParticle; }
	AParticleSystemActor* ParticleActor = nullptr;
	AAmbientLightActor* AmbientActor = nullptr;

private:

	UParticleSystem* CachedParticle = nullptr; //참조가 너무 많아서 Getter 따로 만듦
};