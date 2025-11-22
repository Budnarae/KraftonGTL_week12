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
    UParticleLODLevel() = default;
    ~UParticleLODLevel() = default;
    
    GENERATED_REFLECTION_BODY()
    
    // 모듈 배열에 새 모듈을 추가 (에디터 기능에 필수)
    void AddModule(UParticleModule* NewModule);
    
    // 모듈 배열에서 특정 모듈을 제거
    bool RemoveModule(UParticleModule* TargetModule);

    UParticleModuleRequired* GetRequiredModule();
    void CreateRequiredModule();
    void DeleteRequiredModule();

    TArray<UParticleModule*>& GetModule();
    
    // 이 LOD 레벨의 설정이 유효한지 확인
    bool IsValid() const;

    void Serialize(const bool bInIsLoading, JSON& InOutHandle);

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
    
    // 이 LOD 레벨에 포함된 모든 행동/속성 모듈 리스트 (Spawn, Lifetime, Color 등)
    UPROPERTY(EditAnywhere, Category="Arrays")
    TArray<UParticleModule*> Modules{};
    
    // 이 LOD 레벨에서 파티클을 그리는 방식 (Sprite, Mesh, Beam 등)
    UPROPERTY(EditAnywhere, Category="Assets")
    UParticleModuleTypeDataBase* TypeDataModule{};
};
