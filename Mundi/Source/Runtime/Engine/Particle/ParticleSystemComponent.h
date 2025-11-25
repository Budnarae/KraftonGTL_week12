#include "ParticleSystem.h"           // 템플릿 정의
#include "PrimitiveComponent.h"
#include "ParticleEventTypes.h"
#include "DynamicMeshBuffer.h"
#include "UParticleSystemComponent.generated.h"

// 전방 선언: 실제 파티클 데이터를 담는 런타임 인스턴스 구조체
//struct FParticleSystemInstance;

struct FParticleEmitterInstance;
struct FDynamicEmitterRenderData; 

UCLASS(DisplayName="파티클 시스템 컴포넌트", Description="씬에 배치할 수 있는 파티클 컴포넌트입니다.")
class UParticleSystemComponent : public UPrimitiveComponent
{
public:
    UParticleSystemComponent();
    ~UParticleSystemComponent();

    GENERATED_REFLECTION_BODY()

    // ========================================================================
    // 복사 관련 (Duplication)
    // ========================================================================
    void DuplicateSubObjects() override;
    void PostDuplicate() override;

    // Getters
    UParticleSystem* GetTemplate() const;
    // bool GetIsActive() const;
    float GetElapsedTime() const;
    TArray<FParticleEmitterInstance*>& GetSystemInstance();
    const TArray<FParticleEmitterInstance*>& GetSystemInstance() const;
    int32 GetCurrentLODLevel() const;

    // Setters
    void SetTemplate(UParticleSystem* InTemplate);
    // void SetIsActive(bool bInIsActive);
    void SetElapsedTime(float InElapsedTime);
    void SetCurrentLODLevel(const int32 InCurrentLODLevel);

    // 거리 기반 스폰 설정 (0이면 시간 기반, > 0이면 거리 기반)
    void SetDistancePerSpawn(float InDistance) { DistancePerSpawn = InDistance; }
    float GetDistancePerSpawn() const { return DistancePerSpawn; }

    // 빔 타겟 액터 설정 (빔이 이 액터를 향함)
    void SetBeamTargetActor(class AActor* InActor) { BeamTargetActor = InActor; }
    AActor* GetBeamTargetActor() const { return BeamTargetActor; }

    // 빔 소스 액터 설정 (빔이 이 액터에서 시작)
    void SetBeamSourceActor(class AActor* InActor) { BeamSourceActor = InActor; }
    AActor* GetBeamSourceActor() const { return BeamSourceActor; }

    // SystemInstance 배열 관련 함수
    void AddEmitterInstance(FParticleEmitterInstance* Instance);
    bool RemoveEmitterInstance(FParticleEmitterInstance* Instance);
    bool RemoveEmitterInstanceAt(int32 Index);

    // Collision event 관련 함수
    void AddCollisionEvent(const FParticleEventCollideData& CollideEvent);
    void ClearCollisionEvents();
    const TArray<FParticleEventCollideData>& GetCollisionEvents() const { return CollisionEvents; }

    // Death event 관련 함수
    void AddDeathEvent(const FParticleEventDeathData& DeathEvent);
    void ClearDeathEvents();
    const TArray<FParticleEventDeathData>& GetDeathEvents() const { return DeathEvents; }

    // Spawn event 관련 함수
    void AddSpawnEvent(const FParticleEventSpawnData& SpawnEvent);
    void ClearSpawnEvents();
    const TArray<FParticleEventSpawnData>& GetSpawnEvents() const { return SpawnEvents; }

    // 모든 이벤트 클리어
    void ClearAllEvents();

    // 렌더링 관련 함수
    void CollectMeshBatches(TArray<struct FMeshBatchElement>& OutMeshBatchElements, const struct FSceneView* View) override;

    // Dynamic Data 관리 함수
    void CreateDynamicData();
    void ReleaseDynamicData();

private:
    // 타입별 렌더링 헬퍼 함수
    void RenderSpriteParticles(const TArray<struct FParticleInstanceData>& InstanceData, class UMaterialInterface* Material, TArray<struct FMeshBatchElement>& OutMeshBatchElements);
    void RenderMeshParticles(const TArray<struct FParticleInstanceData>& InstanceData, class UMaterialInterface* Material, class UStaticMesh* Mesh, TArray<struct FMeshBatchElement>& OutMeshBatchElements);

public:

    // 시뮬레이션 시작 명령 (파티클 생성 및 타이머 시작)
    void Activate(bool bReset = false);

    // 시뮬레이션 종료 명령 (새 파티클 생성을 중단하고 기존 파티클이 수명을 다하도록 둠)
    void Deactivate();

    // [Tick Phase] 매 프레임 호출되어 DeltaTime만큼 시뮬레이션을 전진시킵니다. (가장 중요)
    void TickComponent(float DeltaTime) override;
    
    // // 모든 파티클을 즉시 중지하고 메모리를 정리합니다. (강제 종료)
    // void KillParticlesAndCleanUp();
    //
    // // 이 컴포넌트가 현재 재생 중인 이펙트를 멈추고 메모리를 해제합니다. (소멸자 등에서 호출)
    // void BeginDestroy();
    
private:
    // [Template] 이 컴포넌트가 재생할 파티클 시스템의 마스터 설계도 (Asset)
    UPROPERTY(EditAnywhere, Category="Assets")
    UParticleSystem* Template{}; 

    // 이펙트가 현재 재생/시뮬레이션 중인지 여부
    // UPROPERTY(EditAnywhere, Category="Basic")
    // bool bIsActive{};

    // 이펙트가 시작된 후 경과된 시간 (Looping, Duration 체크에 사용)
    UPROPERTY(EditAnywhere, Category="Basic")
    float ElapsedTime{};

    // 현재 LOD 레벨
    // TODO: 현재는 최소 구현으로 무조건 LOD == 0이라고 설정한다. 추후 추가 구현 필요
    int32 CurrentLODLevel{};

    // 템플릿을 기반으로 실제 수많은 파티클의 데이터와 상태를 관리하는 내부 런타임 인스턴스
    TArray<FParticleEmitterInstance*> EmitterInstances{};

    // 렌더링용 Dynamic Data (매 프레임 생성)
    // 다형성: FDynamicSpriteEmitterData 또는 FDynamicMeshEmitterData 등이 들어감
    TArray<FDynamicEmitterRenderData*> DynamicEmitterData{};

    // 이번 틱에서 발생한 이벤트들
    TArray<FParticleEventCollideData> CollisionEvents{};
    TArray<FParticleEventDeathData> DeathEvents{};
    TArray<FParticleEventSpawnData> SpawnEvents{};

    // 빔/리본 렌더링용 동적 버퍼 (에미터 인덱스별로 관리)
    TMap<int32, FDynamicMeshBuffer*> BeamBuffers;
    TMap<int32, FDynamicMeshBuffer*> RibbonBuffers;

    // 리본 위치 보간을 위한 이전 프레임 위치
    FVector PreviousWorldLocation = FVector::Zero();
    bool bHasPreviousLocation = false;

    // 거리 기반 스폰 (0이면 시간 기반, > 0이면 해당 거리마다 1개 스폰)
    float DistancePerSpawn = 0.0f;
    float AccumulatedDistance = 0.0f;

    // 빔 타겟/소스 액터 (빔이 동적으로 이 액터들을 추적)
    AActor* BeamTargetActor = nullptr;
    AActor* BeamSourceActor = nullptr;
};
