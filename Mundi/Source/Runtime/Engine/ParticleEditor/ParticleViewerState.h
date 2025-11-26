#pragma once
#include "Source/Runtime/Engine/Particle/ParticleSystem.h"
#include "Source/Runtime/Engine/GameFramework/ParticleSystemActor.h"
#include "Source/Runtime/Engine/GameFramework/AmbientLightActor.h"
class UWorld; class FViewport; class FViewportClient; class UParticleLODLevel;

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
	int32 SelectedLODLevel = 0;  // 현재 편집 중인 LOD 레벨

	// 선택된 LOD 레벨 접근자
	int32 GetSelectedLODLevel() const { return SelectedLODLevel; }
	void SetSelectedLODLevel(int32 InLODLevel);

	// 선택된 에미터의 현재 LOD 레벨 가져오기
	UParticleLODLevel* GetSelectedLODLevelInstance() const;

	const char* GetPathConstChar() { return ParticlePath.empty() ? "None" : ParticlePath.c_str(); }
	void LoadCachedParticle(const FString& Path);
	void SaveCachedParticle();
	void ReStartParticle();
	void SetLOD(int MoveOffset);

	const UParticleSystem* GetCachedParticle()const { return CachedParticle; }
	UParticleSystem* GetCachedParticle() { return CachedParticle; }
	AParticleSystemActor* ParticleActor = nullptr;
	AAmbientLightActor* AmbientActor = nullptr;

private:

	UParticleSystem* CachedParticle = nullptr; //참조가 너무 많아서 Getter 따로 만듦
};