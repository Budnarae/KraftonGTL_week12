#include "pch.h"
#include "ParticleEmitterInstance.h"
#include "ParticleEmitter.h"
#include "ParticleModuleRequired.h"
#include "ParticleModuleTypeDataBase.h"
#include "ParticleHelper.h"

FParticleEmitterInstance::~FParticleEmitterInstance()
{
    KillAllParticles();
}

// ============================================================================
// 복사 생성자
// ============================================================================
// ParticleData 메모리 블록을 깊은 복사합니다.
// OwnerComponent는 복사 후 호출자가 별도로 설정해야 합니다.
// ============================================================================
FParticleEmitterInstance::FParticleEmitterInstance(const FParticleEmitterInstance& Other)
    : ParticleData(nullptr)
    , OwnerComponent(nullptr)
{
    CopyFrom(Other);
}

// ============================================================================
// 복사 대입 연산자
// ============================================================================
FParticleEmitterInstance& FParticleEmitterInstance::operator=(const FParticleEmitterInstance& Other)
{
    if (this != &Other)
    {
        // 기존 ParticleData 해제
        if (ParticleData)
        {
            free(ParticleData);
            ParticleData = nullptr;
        }
        CopyFrom(Other);
    }
    return *this;
}

// ============================================================================
// 복사 헬퍼 함수
// ============================================================================
void FParticleEmitterInstance::CopyFrom(const FParticleEmitterInstance& Other)
{
    // 템플릿 및 LOD 참조 (얕은 복사 - 공유 데이터)
    SpriteTemplate = Other.SpriteTemplate;
    CurrentLODLevelIndex = Other.CurrentLODLevelIndex;
    CurrentLODLevel = Other.CurrentLODLevel;

    // 에미터 타입 복사
    EmitterType = Other.EmitterType;

    // 메모리 관리 변수 복사
    MaxActiveParticles = Other.MaxActiveParticles;
    ParticleStride = Other.ParticleStride;

    // 런타임 파티클 생성 관리 변수 복사
    SpawnRate = Other.SpawnRate;
    SpawnNum = Other.SpawnNum;
    SpawnFraction = Other.SpawnFraction;
    Duration = Other.Duration;
    ActiveParticles = Other.ActiveParticles;

    // ParticleData 깊은 복사
    if (Other.ParticleData && MaxActiveParticles > 0 && ParticleStride > 0)
    {
        size_t DataSize = static_cast<size_t>(MaxActiveParticles) * ParticleStride;
        ParticleData = static_cast<uint8*>(malloc(DataSize));

        if (ParticleData)
        {
            memcpy(ParticleData, Other.ParticleData, DataSize);
        }
    }
}

void FParticleEmitterInstance::Update(float DeltaTime)
{
    // null 체크
    if (!SpriteTemplate)
        return;

    UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevelInstance();
    if (!LODLevel)
        return;

    UParticleModuleRequired* RequiredModule = LODLevel->GetRequiredModule();
    TArray<UParticleModule*>& UpdateModules = LODLevel->GetUpdateModule();

    for (int32 Index = ActiveParticles - 1; Index >= 0; Index--)
    {
        DECLARE_PARTICLE_PTR(ParticleBase, ParticleData + ParticleStride * Index);

        // FParticleContext 생성
        FParticleContext Context(ParticleBase, OwnerComponent);

        if (RequiredModule)
        {
            RequiredModule->Update(Context, DeltaTime);
        }

        for (UParticleModule* UpdateModule : UpdateModules)
        {
            if (UpdateModule && UpdateModule->GetActive())
            {
                UpdateModule->Update(Context, DeltaTime);
            }
        }

        if (ParticleBase->RelativeTime >= ParticleBase->LifeTime)
            KillParticle(Index);
    }

    float SpawnNumFraction = SpawnRate * DeltaTime + SpawnFraction;
    SpawnNum = floor(SpawnNumFraction);
    SpawnFraction = SpawnNumFraction - SpawnNum;
}

void FParticleEmitterInstance::SpawnParticles
(
    float StartTime,
    float Increment,
    const FVector& PrevLocation,
    const FVector& CurrLocation,
    const FVector& InitialVelocity,
    FParticleEventInstancePayload* EventPayload
)
{
    // null 체크
    if (!SpriteTemplate)
        return;

    UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevelInstance();
    if (!LODLevel)
        return;

    UParticleModuleRequired* RequiredModule = LODLevel->GetRequiredModule();
    TArray<UParticleModule*>& SpawnModules = LODLevel->GetSpawnModule();
    UParticleModuleTypeDataBase* TypeDataModule = LODLevel->GetTypeDataModule();

    // Ribbon/Trail 에미터인지 확인 (파티클 재활용 필요)
    bool bIsRibbonEmitter = TypeDataModule &&
        (TypeDataModule->GetEmitterType() == EDynamicEmitterType::EDET_Ribbon);

    for (int32 Index = 0; Index < SpawnNum; Index++)
    {
        int32 ParticleIndex = ActiveParticles;

        if (ActiveParticles >= MaxActiveParticles)
        {
            if (bIsRibbonEmitter && ActiveParticles > 0)
            {
                // Ribbon: 가장 오래된 파티클을 찾아서 재활용
                // SpawnTime이 가장 작은(오래된) 파티클 찾기
                int32 OldestIndex = 0;
                float OldestTime = FLT_MAX;

                for (int32 i = 0; i < ActiveParticles; ++i)
                {
                    DECLARE_PARTICLE_PTR(CheckParticle, ParticleData + ParticleStride * i);
                    FBaseParticle* BaseCheck = reinterpret_cast<FBaseParticle*>(CheckParticle);

                    // RelativeTime이 가장 큰 파티클 = 가장 오래 살아있는 파티클
                    if (BaseCheck->RelativeTime > OldestTime || OldestTime == FLT_MAX)
                    {
                        OldestTime = BaseCheck->RelativeTime;
                        OldestIndex = i;
                    }
                }

                // 가장 오래된 파티클 위치에 새 파티클 스폰
                ParticleIndex = OldestIndex;
            }
            else
            {
                // 일반 에미터: 그냥 중지
                break;
            }
        }
        else
        {
            ActiveParticles++;
        }

        DECLARE_PARTICLE_PTR(ParticlePtr, ParticleData + ParticleStride * ParticleIndex);

        FBaseParticle& ParticleBase = *((FBaseParticle*)ParticlePtr);

        float LifeTime = GetLifeTimeValue();
        ParticleBase.LifeTime = LifeTime;
        ParticleBase.OneOverMaxLifetime = (LifeTime > 0.0f) ? (1.0f / LifeTime) : 0.0f; // 역수 설정

        // 파티클의 RelativeTime 설정 (Increment를 이용해 시간 분산)
        ParticleBase.RelativeTime = (float)Index * Increment;

        // 이전 위치와 현재 위치 사이를 보간하여 각 파티클이 고유한 위치를 가짐
        float T = (SpawnNum > 1) ? (float)Index / (float)(SpawnNum - 1) : 1.0f;
        FVector InterpolatedLocation = PrevLocation + (CurrLocation - PrevLocation) * T;

        ParticleBase.Location = InterpolatedLocation;
        ParticleBase.OldLocation = InterpolatedLocation; // OldLocation도 초기 위치와 동일하게 설정

        ParticleBase.Velocity = InitialVelocity;
        ParticleBase.BaseVelocity = InitialVelocity; // BaseVelocity는 초기 속도 참조용

        // FParticleContext 생성
        FParticleContext Context(&ParticleBase, OwnerComponent);

        // RequiredModule의 Spawn 호출 (필수)
        if (RequiredModule)
        {
            RequiredModule->Spawn(Context, StartTime);
        }

        // 추가 SpawnModules 호출
        for (UParticleModule* Module : SpawnModules)
        {
            if (Module && Module->GetActive())
            {
                Module->Spawn(Context, StartTime);
            }
        }

        // TypeDataModule의 Spawn 호출 (Ribbon/Beam 등에서 SpawnTime 설정)
        // Index * Increment로 시간 분산하여 각 파티클이 고유한 SpawnTime을 가짐
        if (TypeDataModule)
        {
            TypeDataModule->Spawn(Context, StartTime + (float)Index * Increment);
        }
    }
}

void FParticleEmitterInstance::KillParticle(int32 Index)
{
    if (Index >= ActiveParticles || Index < 0)
    {
        UE_LOG("[FParticleEmitterInstance::KillParticle][Warning] Invalid Index.");
        return;
    }

    ActiveParticles--;

    // 마지막 요소가 아니면 swap-and-pop
    if (Index != ActiveParticles)
    {
        DECLARE_PARTICLE_PTR(Target, ParticleData + ParticleStride * Index);
        DECLARE_PARTICLE_PTR(Last, ParticleData + ParticleStride * ActiveParticles);
        memcpy(Target, Last, ParticleStride);
    }
}

void FParticleEmitterInstance::KillAllParticles()
{
    // KillParticle()이 내부적으로 ActiveParticles--를 하므로
    // 역순으로 순회하면서 마지막 파티클만 계속 제거
    while (ActiveParticles > 0)
    {
        KillParticle(ActiveParticles - 1);
    }
}

float FParticleEmitterInstance::GetLifeTimeValue()
{
    if (!SpriteTemplate)
    {
        UE_LOG("[GetLifeTimeValue] SpriteTemplate is null");
        return 1.0f;  // 기본값
    }

    UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevelInstance();
    if (!LODLevel)
    {
        UE_LOG("[GetLifeTimeValue] LODLevel is null");
        return 1.0f;
    }

    UParticleModuleRequired* RequiredModule = LODLevel->GetRequiredModule();
    if (!RequiredModule)
    {
        UE_LOG("[GetLifeTimeValue] RequiredModule is null");
        return 1.0f;
    }

    return RequiredModule->GetLifeTime();
}