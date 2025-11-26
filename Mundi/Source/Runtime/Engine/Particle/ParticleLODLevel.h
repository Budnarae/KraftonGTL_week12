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

    // TypeDataModule 관련 함수
    UParticleModuleTypeDataBase* GetTypeDataModule() const { return TypeDataModule; }
    void SetTypeDataModule(UParticleModuleTypeDataBase* InTypeDataModule);
    void RemoveTypeDataModule();

    // Find module by type in all module arrays (Spawn, Update)
    template<typename T>
    T* FindModule() const
    {
        static_assert(std::is_base_of_v<UParticleModule, T>, "T must be derived from UParticleModule");

        // Search in SpawnModules
        for (UParticleModule* Module : SpawnModules)
        {
            if (T* Found = Cast<T>(Module))
            {
                return Found;
            }
        }

        // Search in UpdateModules
        for (UParticleModule* Module : UpdateModules)
        {
            if (T* Found = Cast<T>(Module))
            {
                return Found;
            }
        }

        return nullptr;
    }

    void Serialize(const bool bInIsLoading, JSON& InOutHandle);

    // LODDistance 접근자
    float GetLODDistance() const { return LODDistance; }
    void SetLODDistance(float InDistance) { LODDistance = InDistance; }

    // Level 접근자
    int32 GetLevel() const { return Level; }
    void SetLevel(int32 InLevel) { Level = InLevel; }

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

    /**
     * 이 LOD 레벨이 활성화되는 최소 거리
     * - LOD 0: 항상 0.0 (최고 품질, 가장 가까운 거리)
     * - LOD 1+: 이 거리 이상일 때 해당 LOD 사용
     *
     * 예시: LODDistance = 1000.0f → 카메라 거리 1000 이상에서 이 LOD 활성화
     */
    UPROPERTY(EditAnywhere, Category="Basic")
    float LODDistance = 0.0f;

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
