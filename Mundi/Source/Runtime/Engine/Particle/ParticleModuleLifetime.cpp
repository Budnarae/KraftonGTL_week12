#include "pch.h"
#include "ParticleModuleLifetime.h"

#include "ParticleData.h"
#include "ParticleSystemComponent.h"

// ============================================================================
// 생성자
// ============================================================================
UParticleModuleLifetime::UParticleModuleLifetime()
{
    // 기본 수명: 1.0초
    Lifetime.Min = 1.0f;
    Lifetime.Max = 1.0f;
}

// ============================================================================
// Getters
// ============================================================================
const FRawDistribution<float>& UParticleModuleLifetime::GetLifetime() const { return Lifetime; }
float UParticleModuleLifetime::GetLifetimeMin() const { return Lifetime.Min; }
float UParticleModuleLifetime::GetLifetimeMax() const { return Lifetime.Max; }

float UParticleModuleLifetime::GetMaxLifetime() const
{
    // 가능한 최대 수명 반환 (메모리 할당 등에 사용)
    return Lifetime.Max;
}

// ============================================================================
// Setters
// ============================================================================
void UParticleModuleLifetime::SetLifetime(const FRawDistribution<float>& InLifetime) { Lifetime = InLifetime; }
void UParticleModuleLifetime::SetLifetimeMin(float InMin) { Lifetime.Min = InMin; }
void UParticleModuleLifetime::SetLifetimeMax(float InMax) { Lifetime.Max = InMax; }

// ============================================================================
// 헬퍼 함수
// ============================================================================
void UParticleModuleLifetime::SetLifetimeRange(float InMin, float InMax)
{
    Lifetime.Min = InMin;
    Lifetime.Max = InMax;
}

void UParticleModuleLifetime::SetFixedLifetime(float InLifetime)
{
    Lifetime.Min = InLifetime;
    Lifetime.Max = InLifetime;
}

float UParticleModuleLifetime::GetLifetimeValue(float EmitterTime) const
{
    // const 함수에서 Distribution 값을 가져오기 위해 const_cast 사용
    // (GetValue가 const가 아니므로)
    return const_cast<FRawDistribution<float>&>(Lifetime).GetValue(EmitterTime);
}

// ============================================================================
// Spawn 함수
// ============================================================================
// 파티클이 생성될 때 호출되어 수명을 설정합니다.
//
// 처리 순서:
// 1. Distribution에서 수명 값 가져오기
// 2. OneOverMaxLifetime 계산 (성능 최적화)
// 3. 초기 RelativeTime 설정
//
// OneOverMaxLifetime의 역할:
// - Update 시 RelativeTime += DeltaTime * OneOverMaxLifetime
// - 나눗셈을 곱셈으로 변환하여 성능 향상
// - 예: 수명 2초 → OneOverMaxLifetime = 0.5
//       DeltaTime 0.016초 → RelativeTime += 0.008
// ============================================================================
void UParticleModuleLifetime::Spawn(FParticleContext& Context, float EmitterTime)
{
    // ------------------------------------------------------------------------
    // Step 0: 파티클 유효성 검사
    // ------------------------------------------------------------------------
    FBaseParticle* Particle = Context.Particle;
    if (!Particle) return;

    // ------------------------------------------------------------------------
    // Step 1: Distribution에서 수명 값 가져오기
    // ------------------------------------------------------------------------
    // GetValue는 Distribution의 Mode에 따라 다르게 동작:
    // - Uniform 모드: Min~Max 사이 랜덤 값 반환
    // - Curve 모드: EmitterTime에 따른 보간 값 반환
    //
    // EmitterTime 기반이므로:
    // - 이미터 초반에 스폰된 파티클은 긴 수명
    // - 이미터 후반에 스폰된 파티클은 짧은 수명
    // 같은 설정도 가능
    // ------------------------------------------------------------------------
    float MaxLifetime = Lifetime.GetValue(EmitterTime);

    // 수명은 0보다 커야 함
    MaxLifetime = FMath::Max(0.0001f, MaxLifetime);

    // ------------------------------------------------------------------------
    // Step 2: OneOverMaxLifetime 계산
    // ------------------------------------------------------------------------
    // 수명의 역수를 저장하여 Update 시 곱셈으로 처리 가능하게 함
    //
    // Update에서의 사용:
    // RelativeTime += DeltaTime * OneOverMaxLifetime
    // = DeltaTime / MaxLifetime
    //
    // 매 프레임 나눗셈 대신 곱셈을 사용하여 성능 향상
    //
    // 언리얼에서는 여러 Lifetime 모듈이 있으면 수명을 합산하지만,
    // 현재 구현에서는 마지막 모듈의 값을 사용 (덮어쓰기)
    // ------------------------------------------------------------------------
    if (Particle->OneOverMaxLifetime > 0.0f)
    {
        // ------------------------------------------------------------------------
        // 다른 모듈이 이미 수명을 설정한 경우
        // ------------------------------------------------------------------------
        // 언리얼 방식: 수명을 합산
        // 새 수명 = 기존 수명 + 이번 모듈 수명
        //
        // OneOverMaxLifetime에서 기존 수명 역산:
        // 기존 수명 = 1 / OneOverMaxLifetime
        //
        // 합산된 수명의 역수:
        // 새 OneOverMaxLifetime = 1 / (기존 수명 + MaxLifetime)
        // ------------------------------------------------------------------------
        float ExistingLifetime = 1.0f / Particle->OneOverMaxLifetime;
        Particle->OneOverMaxLifetime = 1.0f / (ExistingLifetime + MaxLifetime);
    }
    else
    {
        // ------------------------------------------------------------------------
        // 첫 번째로 수명을 설정하는 경우
        // ------------------------------------------------------------------------
        Particle->OneOverMaxLifetime = 1.0f / MaxLifetime;
    }

    // ------------------------------------------------------------------------
    // Step 3: LifeTime 필드 설정 (참조용)
    // ------------------------------------------------------------------------
    // 일부 시스템에서 직접 수명 값을 참조할 때 사용
    // ------------------------------------------------------------------------
    Particle->LifeTime = MaxLifetime;

    // ------------------------------------------------------------------------
    // Step 4: 초기 RelativeTime 설정
    // ------------------------------------------------------------------------
    // RelativeTime은 파티클의 수명 진행도를 나타냄
    // - 0.0: 막 스폰됨
    // - 1.0: 수명 끝 (제거 대상)
    //
    // 스폰 시점에서는 0.0으로 설정
    // 이미 1.0 이상인 경우는 다른 모듈에서 죽은 것으로 마킹한 것이므로
    // 건드리지 않음
    //
    // 참고: 언리얼에서는 SpawnTime * OneOverMaxLifetime을 설정하여
    // 프레임 내 스폰 시간 분산을 반영하지만, 현재는 단순화하여 0으로 설정
    // ------------------------------------------------------------------------
    if (Particle->RelativeTime < 1.0f)
    {
        Particle->RelativeTime = 0.0f;
    }

    // ------------------------------------------------------------------------
    // 디버그 정보 (필요시 활성화)
    // ------------------------------------------------------------------------
    // UE_LOG("Lifetime Module: Particle lifetime set to %.2f seconds (OneOver: %.4f)",
    //        MaxLifetime, Particle->OneOverMaxLifetime);
}
