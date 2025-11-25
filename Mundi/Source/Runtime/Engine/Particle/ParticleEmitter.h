#pragma once

#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"
#include "ParticleVariable.h"
#include "UParticleEmitter.generated.h"

class UParticleAsset;
class UParticleLODLevel;
UCLASS(DisplayName="파티클 에미터", Description="파티클 LOD 레벨을 저장하는 객체입니다.")
class UParticleEmitter : public UObject
{
public:
    UParticleEmitter();
    ~UParticleEmitter() = default;
    
    GENERATED_REFLECTION_BODY()

    // [셋업 단계] 모든 모듈을 순회하며 파티클 크기(ParticleSize) 및 페이로드 오프셋을 계산하고 캐시합니다.
    void CacheEmitterModuleInfo();
    
    // 이 에미터의 파티클 시뮬레이션 및 렌더링에 필요한 총 메모리 크기를 반환합니다.
    int64 GetRequiredMemorySize() const;

    uint32 GetCurrentLODLevel();
    void SetCurrentLODLevel(const uint32 InCurrentLODLevel);
    
    // // 에디터에서 새 LOD 레벨을 추가합니다.
    // void AddLODLevel();
    //
    // // 참조로 LOD 레벨을 제거한다.
    // bool RemoveLODLevel(UParticleLODLevel* Target);
    //
    // // index로 LOD 레벨을 제거한다.
    // void RemoveLODLevel(uint32 index);

    UParticleLODLevel* GetParticleLODLevelWithIndex(int32 Index);
    UParticleLODLevel* GetCurrentLODLevelInstance();
    
    int32 GetMaxParticleCount();
    void SetMaxParticleCount(const int32 InMaxParticleCount);

    float GetCalculatedDuration();
    
    bool IsValid() const;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle);

    void SetEmitterName(const FString& InName) { EmitterName = InName; }
    const FString& GetEmitterName() const { return EmitterName; }

private:
    // -------------------------------------------
    // 1. 데이터 멤버 (Data Members)
    // -------------------------------------------

    FString EmitterName;

    inline static constexpr int32 INVALID = -1;
    
    // 처음에는 에미터가 비어있으므로 유효하지 않은 값을 가리킨다.
    int32 CurrentLODLevel = INVALID;

    // 이 에미터가 지원하는 모든 LOD 레벨 리스트 (필수)
    UPROPERTY(EditAnywhere, Category = "Array")
    TArray<UParticleLODLevel*> LODLevels;

    // // 파티클 하나당 차지하는 총 메모리 크기 (Base + Payload + Padding)
    // UPROPERTY(EditAnywhere, Category="Basic")
    int32 ParticleSize{};
    
    // 이 에미터가 가질 수 있는 파티클의 최대 개수 (메모리 Precache를 위한 값)
    UPROPERTY(EditAnywhere, Category="Basic")
    int32 MaxParticleCount; 
};
