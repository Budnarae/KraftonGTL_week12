#include "pch.h"
#include "ParticleModuleTypeDataMesh.h"
#include "StaticMesh.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"

UParticleModuleTypeDataMesh::UParticleModuleTypeDataMesh()
    : UParticleModuleTypeDataBase()
{
    ObjectName = "TypeData Mesh";

    // 기본 메시 설정 (Cube)
    /*Mesh = UResourceManager::GetInstance().Load<UStaticMesh>("Assets/StaticMesh/SM_Cube.obj");
    bCameraFacing = false;
    bOverrideMaterial = false;*/
}
