#pragma once

#include "UParticleModuleVelocity.generated.h"
#include "Statistics.h"

struct FBaseParticle;

// ============================================================================
// UParticleModuleVelocity
// ============================================================================
// 파티클이 스폰될 때 초기 속도를 설정하는 모듈입니다.
//
// 언리얼 엔진의 UParticleModuleVelocity를 참고하여 구현되었습니다.
//
// 주요 기능:
// 1. StartVelocity: 3D 벡터로 정의된 초기 속도 (X, Y, Z 방향)
// 2. StartVelocityRadial: 이미터 중심에서 파티클 방향으로의 방사형 속도
// 3. 월드/로컬 공간 변환 지원
// 4. 오너(ParticleSystemComponent) 스케일 적용 옵션
// ============================================================================
UCLASS(DisplayName="파티클 모듈 속도", Description="파티클의 초기 속도를 지정합니다.")
class UParticleModuleVelocity : public UParticleModule
{
public:
    UParticleModuleVelocity() = default;
    ~UParticleModuleVelocity() = default;

    GENERATED_REFLECTION_BODY()

    // ========================================================================
    // UParticleModule 인터페이스 구현
    // ========================================================================

    // Spawn: 파티클이 생성될 때 호출됨
    // - 초기 속도를 계산하여 파티클에 적용
    void Spawn(FParticleContext& Context, float EmitterTime) override;

    // Update는 구현하지 않음 - 속도는 스폰 시에만 설정
    // (속도의 시간에 따른 변화는 VelocityOverLife 모듈에서 담당)

    // ========================================================================
    // Getters
    // ========================================================================
    const FRawDistribution<FVector>& GetStartVelocity() const;
    const FRawDistribution<float>& GetStartVelocityRadial() const;
    bool GetInWorldSpace() const;
    bool GetApplyOwnerScale() const;

    // ========================================================================
    // Setters
    // ========================================================================
    void SetStartVelocity(const FRawDistribution<FVector>& InVelocity);
    void SetStartVelocityMin(const FVector& InMin);
    void SetStartVelocityMax(const FVector& InMax);
    void SetStartVelocityRadial(const FRawDistribution<float>& InRadial);
    void SetStartVelocityRadialMin(float InMin);
    void SetStartVelocityRadialMax(float InMax);
    void SetInWorldSpace(bool bInWorld);
    void SetApplyOwnerScale(bool bApply);

    // ========================================================================
    // 헬퍼 함수
    // ========================================================================

    // 간편하게 속도 범위를 설정하는 함수
    void SetVelocityRange(const FVector& InMin, const FVector& InMax);

    // 단일 방향으로 속도를 설정하는 함수 (랜덤 범위 없음)
    void SetVelocityDirection(const FVector& Direction, float Speed);

private:
    // ========================================================================
    // 속도 관련 프로퍼티
    // ========================================================================

    // ------------------------------------------------------------------------
    // StartVelocity (시작 속도)
    // ------------------------------------------------------------------------
    // 파티클이 스폰될 때 적용되는 초기 속도 벡터입니다.
    //
    // Distribution의 Mode에 따라:
    // - Uniform 모드: Min~Max 사이에서 랜덤 속도 생성
    // - Curve 모드: EmitterTime에 따라 속도가 변화
    //
    // 예시:
    // - 위로 솟는 파티클: Min=(0,0,5), Max=(0,0,10)
    // - 폭발 효과: Min=(-10,-10,-10), Max=(10,10,10)
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[속도]", Tooltip="파티클의 초기 속도 벡터 (Min/Max 범위)")
    FRawDistribution<FVector> StartVelocity{};

    // ------------------------------------------------------------------------
    // StartVelocityRadial (방사형 속도)
    // ------------------------------------------------------------------------
    // 이미터 중심에서 파티클 위치 방향으로의 속도 성분입니다.
    //
    // 계산 방식:
    // 1. 방향 = Normalize(파티클위치 - 이미터원점)
    // 2. 방사형속도 = 방향 * StartVelocityRadial 값
    //
    // 사용 예시:
    // - 폭발 효과: 양수 값 → 중심에서 바깥으로 퍼짐
    // - 블랙홀 효과: 음수 값 → 바깥에서 중심으로 모임
    //
    // 주의: 파티클 위치가 이미터 원점과 같으면 방향이 0이 됨
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[속도]", Tooltip="이미터 중심에서 파티클 방향으로의 방사형 속도")
    FRawDistribution<float> StartVelocityRadial{};

    // ========================================================================
    // 공간 및 스케일 관련 프로퍼티
    // ========================================================================

    // ------------------------------------------------------------------------
    // bInWorldSpace (월드 공간 플래그)
    // ------------------------------------------------------------------------
    // true: 속도가 월드 공간 기준으로 정의됨
    //       → 이미터가 회전해도 속도 방향이 변하지 않음
    //
    // false: 속도가 이미터 로컬 공간 기준으로 정의됨 (기본값)
    //        → 이미터가 회전하면 속도 방향도 같이 회전함
    //
    // 예시:
    // - 총구 이펙트: false (총이 회전하면 총알 방향도 회전)
    // - 비/눈 이펙트: true (캐릭터가 회전해도 항상 아래로 떨어짐)
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[공간]", Tooltip="true면 월드 공간, false면 로컬 공간 기준")
    bool bInWorldSpace = false;

    // ------------------------------------------------------------------------
    // bApplyOwnerScale (오너 스케일 적용)
    // ------------------------------------------------------------------------
    // true: ParticleSystemComponent의 스케일을 속도에 적용 (기본값)
    //       → 컴포넌트가 2배로 커지면 속도도 2배
    //
    // false: 스케일을 무시하고 원래 속도 값 사용
    //        → 컴포넌트 크기와 관계없이 일정한 속도
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[공간]", Tooltip="ParticleSystemComponent의 스케일을 속도에 적용할지 여부")
    bool bApplyOwnerScale = true;
};
