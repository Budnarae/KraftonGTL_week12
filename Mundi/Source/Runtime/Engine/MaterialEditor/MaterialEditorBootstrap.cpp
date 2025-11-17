#include "pch.h"
#include "MaterialEditorBootstrap.h"

#include "CameraActor.h"
#include "FViewport.h"
#include "FSkeletalViewerViewportClient.h"
#include "Source/Runtime/Engine/GameFramework/World.h"
#include "Source/Runtime/Engine/GameFramework/StaticMeshActor.h"
#include "Source/Runtime/Engine/Components/StaticMeshComponent.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include "Source/Runtime/AssetManagement/StaticMesh.h"
#include "Source/Runtime/Core/Object/ObjectFactory.h"

MaterialEditorState* MaterialEditorBootstrap::CreateEditorState(
    const char* TabName,
    UWorld* InWorld,
    ID3D11Device* InDevice
)
{
    if (!InDevice) return nullptr;

    MaterialEditorState* State = new MaterialEditorState();

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
    State->PreviewActor = State->World->SpawnActor<AStaticMeshActor>();

    // 기본 큐브 메시 로드
    if (State->PreviewActor)
    {
        State->PreviewActor->SetStaticMesh("Content/Resources/Model/Cube.umesh");
    }

    return State;
}

void MaterialEditorBootstrap::DestroyEditorState(MaterialEditorState*& State)
{
    if (!State)
        return;

    // 뷰포트 정리
    if (State->Viewport)
    {
        delete State->Viewport;
        State->Viewport = nullptr;
    }

    // 뷰포트 클라이언트 정리
    if (State->Client)
    {
        delete State->Client;
        State->Client = nullptr;
    }

    // 월드 정리
    if (State->World)
    {
        ObjectFactory::DeleteObject(State->World);
        State->World = nullptr;
    }

    // State 삭제
    delete State;
    State = nullptr;
}
