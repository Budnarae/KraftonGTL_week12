#pragma once

class UParticleEmitter;
class UParticleSystemComponent;
class UParticleLODLevel;

// 특정 UParticleEmitter 템플릿의 활성 시뮬레이션 상태를 담는 구조체
struct FParticleEmitterInstance
{
    FParticleEmitterInstance() = default;
    ~FParticleEmitterInstance();

    // 복사 생성자 (Deep Copy)
    FParticleEmitterInstance(const FParticleEmitterInstance& Other);

    // 복사 대입 연산자 (Deep Copy)
    FParticleEmitterInstance& operator=(const FParticleEmitterInstance& Other);

private:
    // 복사 헬퍼 함수
    void CopyFrom(const FParticleEmitterInstance& Other);

public:
    
    // 템플릿 및 소유자 참조
    UParticleEmitter* SpriteTemplate{};
    UParticleSystemComponent* OwnerComponent{};

    // LOD 및 모듈 참조
    int32 CurrentLODLevelIndex{};
    UParticleLODLevel* CurrentLODLevel{};

    // 메모리 관리 및 상태 변수
    uint8* ParticleData{};          // 모든 파티클의 데이터가 저장된 로우 메모리 블록의 시작 주소.
    int32 MaxActiveParticles{};     // ParticleData 블록의 최대 크기를 결정하는 파티클 수 (메모리 할당 기준)

    int32 ParticleStride{};         // 단일 파티클의 메모리 크기이자, 다음 파티클의 시작 주소로 점프하기 위한 $16$ 바이트 정렬이 적용된 최종 크기 (포인터 연산 기준).
    // int32 PayloadOffset;         // 파티클 데이터 내에서 고정 필드가 끝나고 가변 페이로드 데이터가 시작되는 지점을 표시하는 오프셋 (메모리 분할 기준).
    
    // uint8* InstanceData;
    // int32 InstancePayloadSize;

    // 런타임 파티클 생성 관리
    float SpawnRate{};              // 파티클 스폰 레이트 (초당 개수)
    int32 SpawnNum{};               // 이번 프레임에 스폰할 파티클의 개수
    float SpawnFraction{};          // 다음 프레임에 합산할 소수부 나머지

    float Duration{};
    
    int32 ActiveParticles{};        // 현재 시뮬레이션 루프에서 실제 활성화된 파티클의 수 (현재 카운트).

    // 파티클 갱신 함수 (Update 모듈 호출)
    void Update(float DeltaTime);
    
    // 파티클 생성 및 모듈 호출 로직
    void SpawnParticles
    (
        float StartTime,
        float Increment,
        const FVector& InitialLocation, // 충돌 처리 등을 위해 별도의 인자로 받으나 보통 사용되지 않음
        const FVector& InitialVelocity,
        FParticleEventInstancePayload* EventPayload
    );

    // 파티클 소멸 함수
    void KillParticle(int32 Index);

    void KillAllParticles();

    float GetLifeTimeValue()
    {
        return SpriteTemplate->
            GetCurrentLODLevelInstance()->
            GetRequiredModule()->
            GetLifeTime();
    }
};