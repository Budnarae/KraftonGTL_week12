#pragma once

#include "Vector.h"
#include "UPlacementMeshEntry.generated.h"

class UStaticMesh;

UCLASS(DisplayName="Placement Mesh Entry", Description="Entry for weighted mesh placement")
class UPlacementMeshEntry : public UObject
{
public:
    UPlacementMeshEntry() = default;
    ~UPlacementMeshEntry() = default;

    GENERATED_REFLECTION_BODY()

    UPROPERTY(EditAnywhere, Category="Mesh", Tooltip="Static mesh to place")
    UStaticMesh* Mesh = nullptr;

    UPROPERTY(EditAnywhere, Category="Mesh", Tooltip="Weight for random selection (higher = more likely)")
    float Weight = 1.0f;

    // Scale settings per mesh entry
    UPROPERTY(EditAnywhere, Category="Scale", Tooltip="Enable random scale")
    bool bRandomScale = true;

    UPROPERTY(EditAnywhere, Category="Scale", Tooltip="Minimum scale")
    FVector ScaleMin = FVector(0.8f, 0.8f, 0.8f);

    UPROPERTY(EditAnywhere, Category="Scale", Tooltip="Maximum scale")
    FVector ScaleMax = FVector(1.2f, 1.2f, 1.2f);

    UPROPERTY(EditAnywhere, Category="Scale", Tooltip="Uniform scale (use X only)")
    bool bUniformScale = true;
};
