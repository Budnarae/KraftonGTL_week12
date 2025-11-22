#include "pch.h"
#include "ParticleViewerBootstrap.h"
#include "FViewport.h"
#include "ParticleViewerState.h"

ParticleViewerState* ParticleViewerBootstrap::CreateViewerState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice)
{
    if (!InDevice) return nullptr;

    ParticleViewerState* State = new ParticleViewerState();

   
    return State;
}

void ParticleViewerBootstrap::DestroyViewerState(ParticleViewerState*& State)
{
    if (!State) return;
  
    delete State; State = nullptr;
}
