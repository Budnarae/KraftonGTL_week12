#include "ParticleSystem.h"           // 템플릿 정의
#include "SceneComponent.h"
#include "UParticleSystemComponent.generated.h"

// 전방 선언: 실제 파티클 데이터를 담는 런타임 인스턴스 구조체
//struct FParticleSystemInstance; 

struct FParticleEmitterInstance; 

UCLASS(DisplayName="파티클 시스템 컴포넌트", Description="씬에 배치할 수 있는 파티클 컴포넌트입니다.")
class UParticleSystemComponent : public USceneComponent
{
public:
    UParticleSystemComponent() = default;
    ~UParticleSystemComponent() = default;

    GENERATED_REFLECTION_BODY()

    // Getters
    UParticleSystem* GetTemplate() const;
    bool GetIsActive() const;
    float GetElapsedTime() const;
    TArray<FParticleEmitterInstance*>& GetSystemInstance();
    const TArray<FParticleEmitterInstance*>& GetSystemInstance() const;
    int32 GetCurrentLODLevel() const;

    // Setters
    void SetTemplate(UParticleSystem* InTemplate);
    void SetIsActive(bool bInIsActive);
    void SetElapsedTime(float InElapsedTime);
    void SetCurrentLODLevel(const int32 InCurrentLODLevel);

    // SystemInstance 배열 관련 함수
    void AddEmitterInstance(FParticleEmitterInstance* Instance);
    bool RemoveEmitterInstance(FParticleEmitterInstance* Instance);
    bool RemoveEmitterInstanceAt(int32 Index);

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
    UPROPERTY(EditAnywhere, Category="Basic")
    bool bIsActive{};

    // 이펙트가 시작된 후 경과된 시간 (Looping, Duration 체크에 사용)
    UPROPERTY(EditAnywhere, Category="Basic")
    float ElapsedTime{};

    // 현재 LOD 레벨
    // TODO: 현재는 최소 구현으로 무조건 LOD == 0이라고 설정한다. 추후 추가 구현 필요
    int32 CurrentLODLevel{};

    // 템플릿을 기반으로 실제 수많은 파티클의 데이터와 상태를 관리하는 내부 런타임 인스턴스
    TArray<FParticleEmitterInstance*> EmitterInstances{}; 
};
