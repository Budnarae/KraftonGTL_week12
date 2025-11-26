#pragma once

#include "ParticleEmitter.h" // UParticleEmitter를 사용하기 위해 포함
#include "ParticleVariable.h" // EParticleLODMethod
#include "UParticleSystem.generated.h"

UCLASS(DisplayName="파티클 시스템", Description="파티클 데이터 저장의 중추입니다.")
class UParticleSystem : public UObject
{
public:
    UParticleSystem();
    ~UParticleSystem() = default;

    GENERATED_REFLECTION_BODY()
    // -------------------------------------------
    // 2. 핵심 인터페이스 (Core Functions)
    // -------------------------------------------

    // 새 Emitter를 시스템에 추가합니다. (에디터 기능)
    void AddEmitter(UParticleEmitter* NewEmitter);

    // Emitter 리스트에서 특정 Emitter를 제거합니다.
    bool RemoveEmitter(UParticleEmitter* TargetEmitter);

    TArray<UParticleEmitter*>& GetEmitters();

    float GetCalculatedDuration() const;

    // 시스템 전체의 상태가 유효한지 검증합니다.
    bool IsValid() const;

    // // 시스템의 전체 Duration을 Emitters의 설정에 기반하여 계산합니다.
    // float GetCalculatedDuration() const;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle);

    void DuplicateSubObjects() override;

    // -------------------------------------------
    // 3. LOD 관련 인터페이스 (LOD Functions)
    // -------------------------------------------

    // LODMethod Getter/Setter
    EParticleLODMethod GetLODMethod() const { return LODMethod; }
    void SetLODMethod(EParticleLODMethod InMethod) { LODMethod = InMethod; }

    // LODDistanceCheckTime Getter/Setter (초 단위)
    float GetLODDistanceCheckTime() const { return LODDistanceCheckTime; }
    void SetLODDistanceCheckTime(float InTime) { LODDistanceCheckTime = InTime; }

    /**
     * 주어진 거리에 해당하는 LOD 레벨을 반환합니다.
     * 첫 번째 Emitter의 LODLevel 거리 설정을 기준으로 판단합니다.
     *
     * @param Distance 카메라와의 거리
     * @return 해당 거리에 적합한 LOD 레벨 인덱스 (0 = 최고 품질)
     */
    int32 GetLODLevelForDistance(float Distance) const;

public:
    // -------------------------------------------
    // 1. 핵심 데이터 멤버 (Core Data Members)
    // -------------------------------------------

    // 이 시스템을 구성하는 모든 개별 이펙트 요소들(Emitter)의 리스트 (필수)
    // Emitter 배열 덕분에 시스템은 복합적인 시각 효과를 표현할 수 있습니다.
    UPROPERTY(EditAnywhere, Category="Array")
    TArray<UParticleEmitter*> Emitters;

    // 이 시스템이 한 번 재생될 때의 총 재생 시간 (0이면 무한 루프)
    UPROPERTY(EditAnywhere, Category = "Basic")
    float Duration = 0;

    // // 이 시스템의 경계 상자 (Bounding Box) 크기. 컬링(Culling) 최적화에 사용됩니다.
    // UPROPERTY(EditAnywhere, Category="Basic");
    // FAABB FixedBounds;
    //
    // // 시스템 전체에 적용되는 크기 및 속도 배율
    // UPROPERTY(EditAnywhere, Category="Basic");
    // float SystemScale;

private:
    // -------------------------------------------
    // LOD 설정 (LOD Settings)
    // -------------------------------------------

    /**
     * LOD 결정 방식
     * - Automatic: 매 LODDistanceCheckTime 초마다 자동 거리 체크
     * - DirectSet: 게임 코드에서 직접 설정
     * - ActivateAutomatic: 활성화 시점에만 자동 설정
     */
    UPROPERTY(EditAnywhere, Category = "LOD")
    EParticleLODMethod LODMethod = EParticleLODMethod::Automatic;

    /**
     * LOD 거리 체크 주기 (초 단위)
     * - LODMethod가 Automatic일 때만 사용
     * - 기본값 0.25초 (언리얼 기본값)
     */
    UPROPERTY(EditAnywhere, Category = "LOD")
    float LODDistanceCheckTime = 0.25f;
};