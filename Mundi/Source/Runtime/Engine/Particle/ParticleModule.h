#pragma once

#include "ParticleVariable.h"
#include "UParticleModule.generated.h"

struct FBaseParticle;
struct FParticleContext;

UCLASS(DisplayName="파티클 모듈", Description="파티클을 조작하는 기능을 맡습니다.")
class UParticleModule : public UObject
{
public:
    UParticleModule();
    UParticleModule(int32 InPayloadSize);
    ~UParticleModule() = default;

    GENERATED_REFLECTION_BODY()

    // -------------------------------------------
    // 핵심 인터페이스 (Core Virtual Functions)
    // -------------------------------------------

    // [Spawn Phase] 파티클 생성 시점에 딱 한 번 호출되어 초기 속성을 설정합니다.
    virtual void Spawn(FParticleContext& Context, float EmitterTime);

    // [Update Phase] 파티클이 살아있는 동안 매 프레임 호출되어 속성을 갱신합니다.
    virtual void Update(FParticleContext& Context, float DeltaTime);

    // -------------------------------------------
    // 페이로드(Payload) 및 메모리 관리 인터페이스
    // -------------------------------------------

    // 이 모듈이 파티클 하나당 요구하는 추가 메모리(Payload)의 크기를 반환합니다.
    int32 GetRequiredPayloadSize() const;

    // 에미터 설정 단계에서 계산된, 이 모듈의 페이로드 데이터 시작 지점 (Offset)을 저장합니다.
    int32 GetPayloadOffset() const;
    void SetPayloadOffset(int32 NewOffset);

    // -------------------------------------------
    // 활성화 상태 (Activation State)
    // -------------------------------------------

    virtual void SetActive(const bool InActive) { bActive = InActive; }
    bool GetActive() const { return bActive; }

    // -------------------------------------------
    // LOD별 활성화 (Per-LOD Activation)
    // -------------------------------------------

    /**
     * 특정 LOD 레벨에서 이 모듈이 활성화되어 있는지 확인합니다.
     * bActive가 false이면 모든 LOD에서 비활성화됩니다.
     *
     * @param LODIndex 확인할 LOD 레벨 인덱스 (0 = 최고 품질)
     * @return 해당 LOD에서 모듈이 활성화되어 있으면 true
     */
    bool IsEnabledInLOD(int32 LODIndex) const;

    /**
     * 특정 LOD 레벨에서 이 모듈의 활성화 상태를 설정합니다.
     *
     * @param LODIndex 설정할 LOD 레벨 인덱스
     * @param bEnabled 활성화 여부
     */
    void SetEnabledInLOD(int32 LODIndex, bool bEnabled);

    /**
     * 모든 LOD 레벨에서 이 모듈을 활성화합니다.
     */
    void EnableInAllLODs();

    /**
     * 지정된 LOD 레벨 이상에서만 이 모듈을 활성화합니다.
     * 예: EnableAboveLOD(2) → LOD 0, 1에서만 활성화, LOD 2+ 비활성화
     *
     * @param MaxLOD 이 LOD 이상에서는 비활성화 (exclusive)
     */
    void EnableBelowLOD(int32 MaxLOD);

    /**
     * 모듈의 전체 활성 상태(bActive)와 LOD별 활성 상태를 모두 고려하여
     * 특정 LOD에서 실제로 동작해야 하는지 확인합니다.
     *
     * @param LODIndex 확인할 LOD 레벨 인덱스
     * @return 실제로 동작해야 하면 true
     */
    bool ShouldExecuteInLOD(int32 LODIndex) const;

    void Serialize(const bool bInIsLoading, JSON& InOutHandle);

protected:
    // 계산된 페이로드 오프셋을 저장하는 변수 (Payload 접근 시 사용됨)
    int32 PayloadOffset{};
    int32 PayloadSize{};

    // 모듈 전체 활성화 상태
    UPROPERTY(EditAnywhere, Category = "Active")
    bool bActive = true;

    /**
     * LOD별 활성화 플래그 배열
     * - 인덱스 = LOD 레벨 (0 = 최고 품질)
     * - true = 해당 LOD에서 활성화
     * - 기본값: 모든 LOD에서 활성화
     *
     * 예시:
     *   bEnabledInLOD = {true, true, false, false, ...}
     *   → LOD 0, 1에서만 실행, LOD 2 이상에서는 스킵
     */
    bool bEnabledInLOD[MAX_PARTICLE_LODLEVEL_CONST];
};
