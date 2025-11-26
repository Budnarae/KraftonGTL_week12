#pragma once

#include "ParticleModuleTypeDataBase.h"
#include "UParticleModuleTypeDataMesh.generated.h"

class UStaticMesh;

/**
 * TypeData module for mesh particle emitters
 * Automatically sets emitter type to EDET_Mesh when added to LODLevel
 */
UCLASS(DisplayName="메시 타입 데이터", Description="파티클을 스태틱 메시로 렌더링합니다.")
class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase
{
public:
    UParticleModuleTypeDataMesh();
    virtual ~UParticleModuleTypeDataMesh() = default;

    GENERATED_REFLECTION_BODY()

    // TypeData interface
    virtual EDynamicEmitterType GetEmitterType() const override { return EDynamicEmitterType::EDET_Mesh; }
    virtual bool RequiresSpecialRendering() const override { return true; }

    // Mesh accessors
    UStaticMesh* GetMesh() const { return Mesh; }
    void SetMesh(UStaticMesh* InMesh) { Mesh = InMesh; }

    // Camera facing option
    bool IsCameraFacing() const { return bCameraFacing; }
    void SetCameraFacing(bool bInCameraFacing) { bCameraFacing = bInCameraFacing; }

private:
    /**
     * The static mesh to render for particles
     */
    UPROPERTY(EditAnywhere, Category="Mesh")
    UStaticMesh* Mesh = nullptr;

    /**
     * If true, the mesh will face the camera
     */
    UPROPERTY(EditAnywhere, Category="Mesh")
    bool bCameraFacing = false;

    /**
     * If true, use the mesh's material instead of RequiredModule's material
     */
    UPROPERTY(EditAnywhere, Category="Mesh")
    bool bOverrideMaterial = false;
};
