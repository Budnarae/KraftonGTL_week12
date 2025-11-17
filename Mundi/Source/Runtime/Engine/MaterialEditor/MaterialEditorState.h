#pragma once

#include "Source/Runtime/Core/Containers/UEContainer.h"
#include "Windows/SViewerWindowBase.h"

class UWorld;
class FViewport;
class FViewportClient;
class AStaticMeshActor;
class UMaterialInstanceDynamic;
class UMaterial;

enum class EPreviewMeshType : uint8
{
    Sphere = 0,
    Cube,
    Cylinder,
    Plane
};

/**
 * 머티리얼 에디터의 탭별 상태
 */
class MaterialEditorState : public ViewerTabStateBase
{
public:
    MaterialEditorState() = default;
    ~MaterialEditorState() = default;

    // 프리뷰 월드 및 뷰포트
    UWorld* World = nullptr;
    FViewport* Viewport = nullptr;
    FViewportClient* Client = nullptr;

    // 프리뷰 액터
    AStaticMeshActor* PreviewActor = nullptr;

    // 편집 중인 머티리얼
    UMaterialInstanceDynamic* MaterialInstance = nullptr;
    UMaterial* BaseMaterial = nullptr;
    FString LoadedMaterialPath;

    // UI 상태
    char MaterialPathBuffer[260] = {};
    EPreviewMeshType PreviewMeshType = EPreviewMeshType::Sphere;

    // 파라미터 편집 버퍼
    TMap<FString, float> ScalarParameters;
    TMap<FString, FVector> VectorParameters;
};
