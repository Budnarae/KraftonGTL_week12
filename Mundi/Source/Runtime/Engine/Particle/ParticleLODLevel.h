#pragma once

#include "ParticleModule.h"
#include "UParticleLODLevel.generated.h"

// 전방 선언 (Forward Declarations)
class UParticleModuleRequired;
class UParticleModuleTypeDataBase; 

UCLASS(DisplayName="파티클 LOD 레벨", Description="파티클 모듈을 포함하는 객체입니다.")
class UParticleLODLevel : public UObject
{
public:
    UParticleLODLevel();
    ~UParticleLODLevel() = default;
    
    GENERATED_REFLECTION_BODY()
    
    // SpawnModule 관련 함수
    void AddSpawnModule(UParticleModule* NewModule);
    bool RemoveSpawnModule(UParticleModule* TargetModule);
    TArray<UParticleModule*>& GetSpawnModule();

    // UpdateModule 관련 함수
    void AddUpdateModule(UParticleModule* NewModule);
    bool RemoveUpdateModule(UParticleModule* TargetModule);
    TArray<UParticleModule*>& GetUpdateModule();

    // RequiredModule 관련 함수
    UParticleModuleRequired* GetRequiredModule();
    void CreateRequiredModule();
    void DeleteRequiredModule();
    
    // 이 LOD 레벨의 설정이 유효한지 확인
    bool IsValid() const;
private:
    // -------------------------------------------
    // 데이터 멤버 (Data Members)
    // -------------------------------------------

    // 이 LOD 레벨의 인덱스 (0이 최고 품질, 숫자가 클수록 저품질)
    UPROPERTY(EditAnywhere, Category="Basic")
    int32 Level{};
    
    // 이 LOD 레벨을 사용할지 여부
    UPROPERTY(EditAnywhere, Category="Basic")
    bool bEnabled{};

    // 이 LOD 레벨에서만 사용되는 필수 렌더링 설정 (재질, 정렬 등)
    UPROPERTY(EditAnywhere, Category="Assets")
    UParticleModuleRequired* RequiredModule{};  

    // 처음 소환시의 Spawn 동작을 정의
    UPROPERTY(EditAnywhere, Category="Arrays")
    TArray<UParticleModule*> SpawnModules{};
    
    // 매 틱마다의 Update 동작을 정의
    UPROPERTY(EditAnywhere, Category="Arrays")
    TArray<UParticleModule*> UpdateModules{};
    
    // 이 LOD 레벨에서 파티클을 그리는 방식 (Sprite, Mesh, Beam 등)
    UPROPERTY(EditAnywhere, Category="Assets")
    UParticleModuleTypeDataBase* TypeDataModule{};
};
