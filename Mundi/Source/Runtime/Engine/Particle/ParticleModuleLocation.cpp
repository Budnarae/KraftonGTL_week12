﻿#include "pch.h"
#include "ParticleModuleLocation.h"

#include "pch.h"
#include "ParticleModuleLocation.h"

#include "ParticleData.h"
#include "ParticleSystemComponent.h"

// -------------------------------------------
// Getters
// -------------------------------------------
const FRawDistribution<FVector>& UParticleModuleLocation::GetDistribution() const { return Distribution; }
FVector UParticleModuleLocation::GetDistributionMin() const { return Distribution.Min; }
FVector UParticleModuleLocation::GetDistributionMax() const { return Distribution.Max; }
float UParticleModuleLocation::GetDistributeOverNPoints() const { return DistributeOverNPoints; }
float UParticleModuleLocation::GetDistributionThreshold() const { return DistributionThreshold; }

// -------------------------------------------
// Setters
// -------------------------------------------
void UParticleModuleLocation::SetDistribution(const FRawDistribution<FVector>& InDistribution) { Distribution = InDistribution; }
void UParticleModuleLocation::SetDistributionMin(const FVector& InMin) { Distribution.Min = InMin; }
void UParticleModuleLocation::SetDistributionMax(const FVector& InMax) { Distribution.Max = InMax; }
void UParticleModuleLocation::SetDistributeOverNPoints(float InValue) { DistributeOverNPoints = InValue; }
void UParticleModuleLocation::SetDistributionThreshold(float InValue) { DistributionThreshold = InValue; }

// -------------------------------------------
// 헬퍼 함수
// -------------------------------------------
void UParticleModuleLocation::SetDistributionRange(const FVector& InMin, const FVector& InMax)
{
    Distribution.Min = InMin;
    Distribution.Max = InMax;
}

void UParticleModuleLocation::SetDistributionBox(const FVector& Center, const FVector& HalfExtent)
{
    Distribution.Min = Center - HalfExtent;
    Distribution.Max = Center + HalfExtent;
}

void UParticleModuleLocation::SetDistributionSphere(const FVector& Center, float Radius)
{
    FVector Extent(Radius, Radius, Radius);
    Distribution.Min = Center - Extent;
    Distribution.Max = Center + Extent;
}

void UParticleModuleLocation::Spawn(FParticleContext& Context, float EmitterTime)
{
    FBaseParticle* Particle = Context.Particle;
    if (!Particle) return;

    // 최종 위치 오프셋을 저장
    FVector FinalOffset;

    // DistributeOverNPoints가 0이나 1이 아닐 경우에만 균일 분산 로직을 활성화
    if (DistributeOverNPoints > 1.0f)
    {
        // EmitterTime의 소수부를 곱하여 난수의 무작위성을 높인다.
        float RandomNum = FMath::GetRandZeroOneRange();// * FMath::GetFractional(EmitterTime);

        // 어느 분산을 사용할지 결정
        // 일반 분산 사용
        if (RandomNum > DistributionThreshold)
        {
            FinalOffset = Distribution.GetValue(EmitterTime);
        }
        // 균일 분산 사용
        else
        {
            // N개 지점 중 하나를 선택하는 인덱스 계산
            // N-1 범위 내의 정수 인덱스를 무작위로 선택합니다.
            // 예: N=10이면 0~9 중 하나의 인덱스를 선택
            float IndexRange = floorf(DistributeOverNPoints) - 1.0f;

            // 각 축에 독립적인 균일 분산 적용
            auto GetUniformValue = [IndexRange](float Min, float Max) -> float
            {
                float RandomIndexFloat = FMath::GetRandZeroOneRange() * IndexRange;
                int SelectedIndex = FMath::FloorToInt(RandomIndexFloat + 0.5f);
                float LerpRatio = (float)SelectedIndex / IndexRange;
                return FMath::Lerp(Min, Max, LerpRatio);
            };

            FinalOffset = FVector(
                GetUniformValue(Distribution.Min.X, Distribution.Max.X),
                GetUniformValue(Distribution.Min.Y, Distribution.Max.Y),
                GetUniformValue(Distribution.Min.Z, Distribution.Max.Z)
            );
        }
    }
    else
    {
        // 2. 균일 분산을 사용하지 않는 경우 - GetValue로 모드에 따라 처리
        FinalOffset = Distribution.GetValue(EmitterTime);
    }

    FinalOffset = FinalOffset * Context.Owner->GetWorldTransform().ToRotationScaleMatrix();

    // 최종 위치 적용
    Particle->Location = Particle->Location + FinalOffset;
}
