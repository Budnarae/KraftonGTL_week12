#include "pch.h"
#include "ParticleData.h"

#include "ParticleEmitter.h"
#include "ParticleEmitterInstance.h"
#include "ParticleModuleRequired.h"
#include "ParticleModuleTypeDataBase.h"
#include "ParticleHelper.h"

#include <algorithm>

// ============================================================================
// FDynamicSpriteEmitterData 구현
// ============================================================================

FDynamicSpriteEmitterData::~FDynamicSpriteEmitterData()
{
    Release();
}

// ----------------------------------------------------------------------------
// Init - EmitterInstance로부터 렌더링 데이터 초기화
// ----------------------------------------------------------------------------
void FDynamicSpriteEmitterData::Init(FParticleEmitterInstance* Instance, int32 Index)
{
    if (!Instance)
    {
        return;
    }

    EmitterIndex = Index;

    // 기본 정보 설정
    Source.eEmitterType = EDET_Sprite;
    Source.ActiveParticleCount = Instance->ActiveParticles;
    Source.ParticleStride = Instance->ParticleStride;

    // DataContainer 설정 (싱글 스레드이므로 직접 참조)
    Source.DataContainer.ParticleData = Instance->ParticleData;
    Source.DataContainer.ParticleDataNumBytes = Instance->ActiveParticles * Instance->ParticleStride;

    // 인덱스 배열 할당/재할당
    if (Source.DataContainer.ParticleIndices)
    {
        free(Source.DataContainer.ParticleIndices);
        Source.DataContainer.ParticleIndices = nullptr;
    }

    if (Instance->ActiveParticles > 0)
    {
        Source.DataContainer.ParticleIndicesNumShorts = Instance->ActiveParticles;
        Source.DataContainer.ParticleIndices = static_cast<uint16*>(
            malloc(Instance->ActiveParticles * sizeof(uint16))
        );

        // 기본 순서로 인덱스 초기화
        for (int32 i = 0; i < Instance->ActiveParticles; ++i)
        {
            Source.DataContainer.ParticleIndices[i] = static_cast<uint16>(i);
        }
    }

    // 머티리얼 정보 설정
    if (Instance->CurrentLODLevel && Instance->CurrentLODLevel->GetRequiredModule())
    {
        Source.MaterialInterface = Instance->CurrentLODLevel->GetRequiredModule()->GetMaterial();
    }
    else
    {
        Source.MaterialInterface = nullptr;
    }
}

// ----------------------------------------------------------------------------
// SortParticles - 카메라 거리 기준 Back-to-Front 정렬
// ----------------------------------------------------------------------------
void FDynamicSpriteEmitterData::SortParticles(const FVector& CameraPosition)
{
    if (Source.ActiveParticleCount <= 1 || !Source.DataContainer.ParticleIndices)
    {
        return;
    }

    uint8* ParticleData = Source.DataContainer.ParticleData;
    int32 Stride = Source.ParticleStride;
    uint16* Indices = Source.DataContainer.ParticleIndices;
    int32 Count = Source.ActiveParticleCount;

    // 각 파티클의 카메라 거리 제곱 계산
    TArray<float> DistancesSq;
    DistancesSq.resize(Count);

    for (int32 i = 0; i < Count; ++i)
    {
        FBaseParticle* Particle = reinterpret_cast<FBaseParticle*>(ParticleData + i * Stride);
        FVector Diff = Particle->Location - CameraPosition;
        DistancesSq[i] = Diff.X * Diff.X + Diff.Y * Diff.Y + Diff.Z * Diff.Z;
    }

    // 인덱스 배열을 거리 기준으로 정렬 (먼 것부터 - Back-to-Front)
    // std::sort 사용 - O(n log n) 복잡도
    std::sort(Indices, Indices + Count, [&DistancesSq](uint16 A, uint16 B)
    {
        return DistancesSq[A] > DistancesSq[B]; // 내림차순 (먼 것부터)
    });
}

// ----------------------------------------------------------------------------
// Release - 할당된 메모리 해제
// ----------------------------------------------------------------------------
void FDynamicSpriteEmitterData::Release()
{
    if (Source.DataContainer.ParticleIndices)
    {
        free(Source.DataContainer.ParticleIndices);
        Source.DataContainer.ParticleIndices = nullptr;
    }

    // ParticleData는 EmitterInstance가 소유하므로 여기서 해제하지 않음
    Source.DataContainer.ParticleData = nullptr;
    Source.ActiveParticleCount = 0;
}

// ============================================================================
// FDynamicMeshEmitterData 구현
// ============================================================================

FDynamicMeshEmitterData::~FDynamicMeshEmitterData()
{
    Release();
}

// ----------------------------------------------------------------------------
// Init - EmitterInstance로부터 렌더링 데이터 초기화 (메시 에미터용)
// ----------------------------------------------------------------------------
void FDynamicMeshEmitterData::Init(FParticleEmitterInstance* Instance, int32 Index)
{
    if (!Instance)
    {
        return;
    }

    EmitterIndex = Index;

    // 기본 정보 설정 (타입은 EDET_Mesh로!)
    Source.eEmitterType = EDET_Mesh;
    Source.ActiveParticleCount = Instance->ActiveParticles;
    Source.ParticleStride = Instance->ParticleStride;

    // DataContainer 설정 (싱글 스레드이므로 직접 참조)
    Source.DataContainer.ParticleData = Instance->ParticleData;
    Source.DataContainer.ParticleDataNumBytes = Instance->ActiveParticles * Instance->ParticleStride;

    // 인덱스 배열 할당/재할당
    if (Source.DataContainer.ParticleIndices)
    {
        free(Source.DataContainer.ParticleIndices);
        Source.DataContainer.ParticleIndices = nullptr;
    }

    if (Instance->ActiveParticles > 0)
    {
        Source.DataContainer.ParticleIndicesNumShorts = Instance->ActiveParticles;
        Source.DataContainer.ParticleIndices = static_cast<uint16*>(
            malloc(Instance->ActiveParticles * sizeof(uint16))
        );

        // 기본 순서로 인덱스 초기화
        for (int32 i = 0; i < Instance->ActiveParticles; ++i)
        {
            Source.DataContainer.ParticleIndices[i] = static_cast<uint16>(i);
        }
    }

    // 머티리얼 및 메시 정보 설정 (RequiredModule에서 가져옴)
    if (Instance->CurrentLODLevel && Instance->CurrentLODLevel->GetRequiredModule())
    {
        UParticleModuleRequired* RequiredModule = Instance->CurrentLODLevel->GetRequiredModule();
        Source.MaterialInterface = RequiredModule->GetMaterial();
        StaticMesh = RequiredModule->GetMesh();
    }
    else
    {
        Source.MaterialInterface = nullptr;
        StaticMesh = nullptr;
    }

    // 스케일 설정
    if (Instance->OwnerComponent)
    {
        Source.Scale = Instance->OwnerComponent->GetWorldScale();
    }
    else
    {
        Source.Scale = FVector(1.0f, 1.0f, 1.0f);
    }

    // 정렬 모드 (기본값: 없음)
    Source.SortMode = 0;
}

// ----------------------------------------------------------------------------
// SortParticles - 카메라 거리 기준 Back-to-Front 정렬 (메시도 동일)
// ----------------------------------------------------------------------------
void FDynamicMeshEmitterData::SortParticles(const FVector& CameraPosition)
{
    if (Source.ActiveParticleCount <= 1 || !Source.DataContainer.ParticleIndices)
    {
        return;
    }

    uint8* ParticleData = Source.DataContainer.ParticleData;
    int32 Stride = Source.ParticleStride;
    uint16* Indices = Source.DataContainer.ParticleIndices;
    int32 Count = Source.ActiveParticleCount;

    // 각 파티클의 카메라 거리 제곱 계산
    TArray<float> DistancesSq;
    DistancesSq.resize(Count);

    for (int32 i = 0; i < Count; ++i)
    {
        FBaseParticle* Particle = reinterpret_cast<FBaseParticle*>(ParticleData + i * Stride);
        FVector Diff = Particle->Location - CameraPosition;
        DistancesSq[i] = Diff.X * Diff.X + Diff.Y * Diff.Y + Diff.Z * Diff.Z;
    }

    // 인덱스 배열을 거리 기준으로 정렬 (먼 것부터 - Back-to-Front)
    std::sort(Indices, Indices + Count, [&DistancesSq](uint16 A, uint16 B)
    {
        return DistancesSq[A] > DistancesSq[B]; // 내림차순 (먼 것부터)
    });
}

// ----------------------------------------------------------------------------
// Release - 할당된 메모리 해제
// ----------------------------------------------------------------------------
void FDynamicMeshEmitterData::Release()
{
    if (Source.DataContainer.ParticleIndices)
    {
        free(Source.DataContainer.ParticleIndices);
        Source.DataContainer.ParticleIndices = nullptr;
    }

    // ParticleData는 EmitterInstance가 소유하므로 여기서 해제하지 않음
    Source.DataContainer.ParticleData = nullptr;
    Source.ActiveParticleCount = 0;

    // StaticMesh는 Asset이므로 해제하지 않음
    StaticMesh = nullptr;
}