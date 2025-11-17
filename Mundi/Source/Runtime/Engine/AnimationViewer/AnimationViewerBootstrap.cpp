#include "pch.h"
#include "AnimationViewerBootstrap.h"

#include "CameraActor.h"
#include "FViewport.h"
#include "FSkeletalViewerViewportClient.h"
#include "Source/Runtime/Engine/GameFramework/World.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Core/Object/ObjectFactory.h"

AnimationViewerState* AnimationViewerBootstrap::CreateViewerState(
    const char* TabName,
    UWorld* InWorld,
    ID3D11Device* InDevice
)
{
    if (!InDevice) return nullptr;

    AnimationViewerState* State = new AnimationViewerState();

    // 프리뷰 월드 생성
    State->World = NewObject<UWorld>();
    State->World->SetWorldType(EWorldType::PreviewMinimal);
    State->World->Initialize();
    State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_EditorIcon);

    // 뷰포트 생성
    State->Viewport = new FViewport();
    State->Viewport->Initialize(0, 0, 1, 1, InDevice);

    // 뷰포트 클라이언트 생성
    auto* Client = new FSkeletalViewerViewportClient();
    Client->SetWorld(State->World);
    Client->SetViewportType(EViewportType::Perspective);
    Client->SetViewMode(EViewMode::VMI_Lit_Phong);
    Client->GetCamera()->SetActorLocation(FVector(3, 0, 2));

    State->Client = Client;
    State->Viewport->SetViewportClient(Client);
    State->World->SetEditorCameraActor(Client->GetCamera());

    // 프리뷰 액터 스폰
    State->PreviewActor = State->World->SpawnActor<ASkeletalMeshActor>();

    return State;
}

void AnimationViewerBootstrap::DestroyViewerState(AnimationViewerState*& State)
{
    if (!State)
        return;

    if (State->Viewport)
    {
        delete State->Viewport;
        State->Viewport = nullptr;
    }

    if (State->Client)
    {
        delete State->Client;
        State->Client = nullptr;
    }

    if (State->World)
    {
        ObjectFactory::DeleteObject(State->World);
        State->World = nullptr;
    }

    delete State;
    State = nullptr;
}
