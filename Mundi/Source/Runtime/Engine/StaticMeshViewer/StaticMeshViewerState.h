#pragma once

class UWorld;
class FViewport;
class FViewportClient;
class AStaticMeshActor;
class UStaticMesh;

/**
 * 스태틱 메시 뷰어 상태
 */
class StaticMeshViewerState
{
public:
    StaticMeshViewerState() = default;
    ~StaticMeshViewerState() = default;

    // 프리뷰 월드 및 뷰포트
    UWorld* World = nullptr;
    FViewport* Viewport = nullptr;
    FViewportClient* Client = nullptr;

    // 프리뷰 액터
    AStaticMeshActor* PreviewActor = nullptr;

    // 현재 로드된 메시
    UStaticMesh* CurrentMesh = nullptr;
    FString LoadedMeshPath;

    // UI 상태
    char MeshPathBuffer[260] = {};
    int32 SelectedLOD = 0;
    bool bShowWireframe = false;
    bool bShowBounds = false;
    bool bShowCollision = false;
};
