#include "pch.h"
#include "ParticleViewerBootstrap.h"
#include "FViewport.h"
#include "ParticleViewerState.h"
#include "Source/Runtime/Renderer/FParticleViewerViewportClient.h"
#include "Source/Runtime/Engine/GameFramework/ParticleSystemActor.h"
#include "Source/Runtime/Engine/GameFramework/AmbientLightActor.h"
#include "Source/Runtime/Engine/Components/AmbientLightComponent.h"
#include "CameraActor.h"

ParticleViewerState* ParticleViewerBootstrap::CreateViewerState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice)
{
    if (!InDevice) return nullptr;

    ParticleViewerState* State = new ParticleViewerState();
    // Preview world 만들기
    State->World = NewObject<UWorld>();
    State->World->SetWorldType(EWorldType::PreviewMinimal);  // Set as preview world for memory optimization
    State->World->Initialize();
    State->World->GetGizmoActor()->SetActorActive(false);
    State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_EditorIcon);

    State->World->GetGizmoActor()->SetSpace(EGizmoSpace::Local);

    State->Viewport = new FViewport();
    State->Viewport->Initialize(0, 0, 1, 1, InDevice);
    auto* Client = new FParticleViewerViewportClient();
    Client->SetWorld(State->World);
    Client->SetViewportType(EViewportType::Perspective);
    Client->SetViewMode(EViewMode::VMI_Lit_Phong);
    Client->GetCamera()->SetActorLocation(FVector(3, 0, 0));

    State->Client = Client;
    State->Viewport->SetViewportClient(Client);
    State->World->SetEditorCameraActor(Client->GetCamera());

    State->ParticleActor = State->World->SpawnActor<AParticleSystemActor>();
    State->ParticleActor->SetTickInEditor(true);
    return State;
}

void ParticleViewerBootstrap::DestroyViewerState(ParticleViewerState*& State)
{
    if (!State) return;
  
    delete State; State = nullptr;
}
