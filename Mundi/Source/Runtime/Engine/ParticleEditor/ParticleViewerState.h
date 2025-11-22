#pragma once

class UWorld; class FViewport; class FViewportClient;

class ParticleViewerState
{
public:
	FViewport* Viewport = nullptr;
	FViewportClient* Client = nullptr;
};