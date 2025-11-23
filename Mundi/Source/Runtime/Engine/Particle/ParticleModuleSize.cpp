#include "pch.h"
#include "ParticleModuleSize.h"

#include "ParticleData.h"
#include "ParticleSystemComponent.h"

// ============================================================================
// 생성자
// ============================================================================
UParticleModuleSize::UParticleModuleSize()
{
    // 기본 크기: (1, 1, 1) 유닛
    StartSize.Min = FVector(1.0f, 1.0f, 1.0f);
    StartSize.Max = FVector(1.0f, 1.0f, 1.0f);
}

// ============================================================================
// Getters
// ============================================================================
const FRawDistribution<FVector>& UParticleModuleSize::GetStartSize() const { return StartSize; }
FVector UParticleModuleSize::GetStartSizeMin() const { return StartSize.Min; }
FVector UParticleModuleSize::GetStartSizeMax() const { return StartSize.Max; }

// ============================================================================
// Setters
// ============================================================================
void UParticleModuleSize::SetStartSize(const FRawDistribution<FVector>& InSize) { StartSize = InSize; }
void UParticleModuleSize::SetStartSizeMin(const FVector& InMin) { StartSize.Min = InMin; }
void UParticleModuleSize::SetStartSizeMax(const FVector& InMax) { StartSize.Max = InMax; }

// ============================================================================
// 헬퍼 함수
// ============================================================================
void UParticleModuleSize::SetSizeRange(const FVector& InMin, const FVector& InMax)
{
    StartSize.Min = InMin;
    StartSize.Max = InMax;
}

void UParticleModuleSize::SetFixedSize(const FVector& InSize)
{
    StartSize.Min = InSize;
    StartSize.Max = InSize;
}

void UParticleModuleSize::SetUniformSize(float InMin, float InMax)
{
    StartSize.Min = FVector(InMin, InMin, InMin);
    StartSize.Max = FVector(InMax, InMax, InMax);
}

void UParticleModuleSize::SetFixedUniformSize(float InSize)
{
    StartSize.Min = FVector(InSize, InSize, InSize);
    StartSize.Max = FVector(InSize, InSize, InSize);
}

// ============================================================================
// Spawn 함수
// ============================================================================
// 파티클이 생성될 때 호출되어 크기를 설정합니다.
//
// 처리 순서:
// 1. Distribution에서 크기 값 가져오기
// 2. Size와 BaseSize에 **누적** (+= 연산)
//
// 누적 방식을 사용하는 이유:
// - 여러 Size 모듈을 조합하여 최종 크기 결정
// - 기본 크기(1,1,1) + 추가 크기로 구성
// - 유연한 효과 조합 가능
//
// 예시:
// 파티클 초기 크기: (0, 0, 0)
// SizeModule1: += (0.5, 0.5, 0.5) → (0.5, 0.5, 0.5)
// SizeModule2: += (0.3, 0.3, 0.3) → (0.8, 0.8, 0.8)
// ============================================================================
void UParticleModuleSize::Spawn(FParticleContext& Context, float EmitterTime)
{
    // ------------------------------------------------------------------------
    // Step 0: 파티클 유효성 검사
    // ------------------------------------------------------------------------
    FBaseParticle* Particle = Context.Particle;
    if (!Particle) return;

    // ------------------------------------------------------------------------
    // Step 1: Distribution에서 크기 값 가져오기
    // ------------------------------------------------------------------------
    // GetValue는 Distribution의 Mode에 따라 다르게 동작:
    // - Uniform 모드: Min~Max 사이 랜덤 크기 반환
    // - Curve 모드: EmitterTime에 따른 보간 크기 반환
    //
    // EmitterTime 기반이므로:
    // - 이미터 초반에 스폰된 파티클은 큰 크기
    // - 이미터 후반에 스폰된 파티클은 작은 크기
    // 같은 설정도 가능
    //
    // 반환값:
    // - X = 가로 크기
    // - Y = 세로 크기
    // - Z = 깊이 (3D 파티클용)
    // ------------------------------------------------------------------------
    FVector SizeVec = StartSize.GetValue(EmitterTime);

    // ------------------------------------------------------------------------
    // Step 2: Size와 BaseSize에 누적
    // ------------------------------------------------------------------------
    // += 연산자를 사용하여 여러 모듈이 누적될 수 있도록 함
    //
    // Size: 현재 프레임의 크기 (Update 모듈에서 변경 가능)
    //       예: SizeOverLife 모듈이 이 값을 매 프레임 수정
    //
    // BaseSize: 초기 크기 (참조용으로 보존)
    //           예: SizeScale 모듈이 BaseSize를 기준으로 스케일링
    //
    // 언리얼에서도 동일하게 += 연산 사용:
    // Particle.Size += (FVector3f)Size;
    // Particle.BaseSize += (FVector3f)Size;
    //
    // 주의: 파티클 초기화 시 Size와 BaseSize가 (0, 0, 0)이므로,
    // 첫 번째 Size 모듈의 값이 실제 초기 크기가 됨
    // ------------------------------------------------------------------------
    Particle->Size += SizeVec;
    Particle->BaseSize += SizeVec;

    // ------------------------------------------------------------------------
    // 디버그 정보 (필요시 활성화)
    // ------------------------------------------------------------------------
    // UE_LOG("Size Module: Particle size increased by (%.2f, %.2f, %.2f), Total: (%.2f, %.2f, %.2f)",
    //        SizeVec.X, SizeVec.Y, SizeVec.Z,
    //        Particle->Size.X, Particle->Size.Y, Particle->Size.Z);
}
