#pragma once

class UWorld; class FViewport; class FViewportClient;

class ParticleViewerState
{
public:
	UWorld* World = nullptr;
	FViewport* Viewport = nullptr;
	FViewportClient* Client = nullptr;

	UParticleAsset* PreviewParticle = nullptr;
	UParticleEmitter* SelectedEmitter = nullptr;
	UParticleModule* SelectedModule = nullptr;
};