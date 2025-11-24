#pragma once

#include "UParticleModule.generated.h"

struct FBaseParticle;
struct FParticleContext; 

UCLASS(DisplayName="파티클 모듈", Description="파티클을 조작하는 기능을 맡습니다.")
class UParticleModule : public UObject
{
public:
    UParticleModule() = default;
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
public:
    // -------------------------------------------
    // 최소 데이터 멤버 (Minimal Data Members)
    // -------------------------------------------

    // // 모듈이 파티클에게 부여할 값의 최소/최대 범위를 위한 변수 (예: 수명, 크기)
    // UPROPERTY(EditAnywhere, Category="Basic")
    // float Value_Min;
    //
    // UPROPERTY(EditAnywhere, Category="Basic")
    // float Value_Max;

    void Serialize(const bool bInIsLoading, JSON& InOutHandle);

protected:
    // 계산된 페이로드 오프셋을 저장하는 변수 (Payload 접근 시 사용됨)
    int32 PayloadOffset{};
    int32 PayloadSize{};
};
