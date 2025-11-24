#include "pch.h"
#include "ParticleModuleVelocity.h"

#include "ParticleData.h"
#include "ParticleSystemComponent.h"

// ============================================================================
// Getters
// ============================================================================
const FRawDistribution<FVector>& UParticleModuleVelocity::GetStartVelocity() const { return StartVelocity; }
const FRawDistribution<float>& UParticleModuleVelocity::GetStartVelocityRadial() const { return StartVelocityRadial; }
bool UParticleModuleVelocity::GetInWorldSpace() const { return bInWorldSpace; }
bool UParticleModuleVelocity::GetApplyOwnerScale() const { return bApplyOwnerScale; }

// ============================================================================
// Setters
// ============================================================================
void UParticleModuleVelocity::SetStartVelocity(const FRawDistribution<FVector>& InVelocity) { StartVelocity = InVelocity; }
void UParticleModuleVelocity::SetStartVelocityMin(const FVector& InMin) { StartVelocity.Min = InMin; }
void UParticleModuleVelocity::SetStartVelocityMax(const FVector& InMax) { StartVelocity.Max = InMax; }
void UParticleModuleVelocity::SetStartVelocityRadial(const FRawDistribution<float>& InRadial) { StartVelocityRadial = InRadial; }
void UParticleModuleVelocity::SetStartVelocityRadialMin(float InMin) { StartVelocityRadial.Min = InMin; }
void UParticleModuleVelocity::SetStartVelocityRadialMax(float InMax) { StartVelocityRadial.Max = InMax; }
void UParticleModuleVelocity::SetInWorldSpace(bool bInWorld) { bInWorldSpace = bInWorld; }
void UParticleModuleVelocity::SetApplyOwnerScale(bool bApply) { bApplyOwnerScale = bApply; }

// ============================================================================
// 헬퍼 함수
// ============================================================================
void UParticleModuleVelocity::SetVelocityRange(const FVector& InMin, const FVector& InMax)
{
    StartVelocity.Min = InMin;
    StartVelocity.Max = InMax;
}

void UParticleModuleVelocity::SetVelocityDirection(const FVector& Direction, float Speed)
{
    // 방향을 정규화하고 속도를 곱함
    // Min과 Max를 같게 설정하여 랜덤 없이 일정한 속도 적용
    FVector NormalizedDir = Direction;
    float Length = Direction.Size();
    if (Length > 0.0001f)
    {
        NormalizedDir = Direction / Length;
    }

    FVector Velocity = NormalizedDir * Speed;
    StartVelocity.Min = Velocity;
    StartVelocity.Max = Velocity;
}

// ============================================================================
// Spawn 함수
// ============================================================================
// 파티클이 생성될 때 호출되어 초기 속도를 설정합니다.
//
// 처리 순서:
// 1. Distribution에서 기본 속도 값 가져오기
// 2. 방사형 방향 계산
// 3. 공간 변환 적용 (월드/로컬)
// 4. 오너 스케일 적용
// 5. 방사형 속도 추가
// 6. 최종 속도를 파티클에 적용
// ============================================================================
void UParticleModuleVelocity::Spawn(FParticleContext& Context, float EmitterTime)
{
    // ------------------------------------------------------------------------
    // Step 0: 파티클 유효성 검사
    // ------------------------------------------------------------------------
    FBaseParticle* Particle = Context.Particle;
    if (!Particle) return;

    // ------------------------------------------------------------------------
    // Step 1: Distribution에서 기본 속도 가져오기
    // ------------------------------------------------------------------------
    // GetValue는 Distribution의 Mode에 따라 다르게 동작:
    // - Uniform 모드: Min~Max 사이 랜덤 값 반환 (EmitterTime 무시)
    // - Curve 모드: EmitterTime에 따른 보간 값 반환
    //
    // 언리얼에서는 EmitterTime을 사용하여 시간에 따라
    // 초기 속도가 변할 수 있도록 함
    // 예: 분수 이펙트에서 시간이 지남에 따라 물줄기 세기가 약해짐
    // ------------------------------------------------------------------------
    FVector Vel = StartVelocity.GetValue(EmitterTime);

    // ------------------------------------------------------------------------
    // Step 2: 방사형 방향 계산
    // ------------------------------------------------------------------------
    // 방사형 속도는 이미터 중심에서 파티클 방향으로 적용됨
    //
    // 계산:
    // FromOrigin = Normalize(파티클위치 - 이미터원점)
    //
    // 이미터 원점은 ParticleSystemComponent의 월드 위치입니다.
    // 파티클 위치는 이미 LocationModule에서 설정된 상태입니다.
    //
    // 주의: 파티클이 이미터 원점에 정확히 위치하면 방향이 0이 됨
    // GetSafeNormal()을 사용하여 길이가 0인 경우 (0,0,0) 반환
    // ------------------------------------------------------------------------
    FVector EmitterOrigin = Context.Owner->GetWorldTransform().Translation;
    FVector FromOrigin = (Particle->Location - EmitterOrigin).GetSafeNormal();

    // ------------------------------------------------------------------------
    // Step 3: 오너 스케일 가져오기
    // ------------------------------------------------------------------------
    // bApplyOwnerScale이 true면 ParticleSystemComponent의 스케일을 적용
    //
    // 이유:
    // - 파티클 시스템이 2배로 스케일되면 속도도 2배가 되어야 자연스러움
    // - 예: 거대 폭발 vs 작은 폭발 - 크기에 비례한 속도
    //
    // 스케일이 비균일할 수 있으므로 FVector로 처리
    // 나중에 속도 벡터의 각 성분에 곱해짐
    // ------------------------------------------------------------------------
    FVector OwnerScale(1.0f, 1.0f, 1.0f);
    if (bApplyOwnerScale)
    {
        OwnerScale = Context.Owner->GetWorldTransform().Scale3D;
    }

    // ------------------------------------------------------------------------
    // Step 4: 공간 변환 처리
    // ------------------------------------------------------------------------
    // 속도 벡터를 적절한 공간으로 변환합니다.
    //
    // bInWorldSpace == false (기본값):
    //   속도가 이미터 로컬 공간에서 정의됨
    //   → 이미터의 회전/스케일 행렬을 적용하여 월드 공간으로 변환
    //   예: 총구 이펙트 - 총이 회전하면 총알 방향도 회전
    //
    // bInWorldSpace == true:
    //   속도가 이미 월드 공간에서 정의됨
    //   → 변환 없이 그대로 사용
    //   예: 비/눈 이펙트 - 항상 (0, 0, -1) 방향으로 떨어짐
    //
    // ToRotationScaleMatrix():
    //   Translation을 제외한 회전과 스케일만 적용
    //   속도는 방향과 크기만 중요하고, 위치 오프셋은 필요 없음
    // ------------------------------------------------------------------------
    if (!bInWorldSpace)
    {
        // 로컬 공간 → 월드 공간 변환
        // 이미터의 회전과 스케일을 속도 벡터에 적용
        Vel = Vel * Context.Owner->GetWorldTransform().ToRotationScaleMatrix();
    }
    // else: 월드 공간이므로 변환 불필요

    // ------------------------------------------------------------------------
    // Step 5: 오너 스케일 적용
    // ------------------------------------------------------------------------
    // 각 축에 대해 스케일을 곱함
    //
    // 참고: ToRotationScaleMatrix()에 이미 스케일이 포함되어 있지만,
    // bInWorldSpace == true인 경우에도 스케일은 적용해야 함
    // 언리얼에서는 이 순서로 처리함
    // ------------------------------------------------------------------------
    Vel = Vel * OwnerScale;

    // ------------------------------------------------------------------------
    // Step 6: 방사형 속도 추가
    // ------------------------------------------------------------------------
    // 방사형 속도 = 방향 * 크기
    //
    // 방향: FromOrigin (이미터에서 파티클로의 단위 벡터)
    // 크기: StartVelocityRadial.GetValue() (Distribution에서 가져온 값)
    //
    // 방사형 속도에도 오너 스케일이 적용됨
    // 단, 방향은 이미 월드 공간이므로 공간 변환은 불필요
    //
    // 결과:
    // - 양수: 바깥으로 퍼지는 폭발 효과
    // - 음수: 안쪽으로 모이는 블랙홀 효과
    // ------------------------------------------------------------------------
    float RadialSpeed = StartVelocityRadial.GetValue(EmitterTime);
    Vel += FromOrigin * RadialSpeed * OwnerScale;

    // ------------------------------------------------------------------------
    // Step 7: 최종 속도를 파티클에 적용
    // ------------------------------------------------------------------------
    // Velocity: 현재 파티클의 속도 (매 프레임 업데이트될 수 있음)
    // BaseVelocity: 초기 속도 (참조용으로 보존)
    //
    // += 연산자를 사용하여 여러 속도 모듈이 누적될 수 있도록 함
    // 예: VelocityModule + VelocityInheritParent = 합쳐진 속도
    //
    // 참고:
    // - RequiredModule의 Update에서 Location += Velocity * DeltaTime
    // - BaseVelocity는 VelocityOverLife 등에서 참조로 사용
    // ------------------------------------------------------------------------
    Particle->Velocity += Vel;
    Particle->BaseVelocity += Vel;
}
