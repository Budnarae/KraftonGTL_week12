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
    for (int32 Index = ActiveParticles - 1; Index >= 0; Index--)
    {
        DECLARE_PARTICLE_PTR(ParticleBase, ParticleData + ParticleStride * Index);

        // FParticleContext 생성
        FParticleContext Context(ParticleBase, OwnerComponent);

        UParticleModuleRequired* RequiredModule = \
            SpriteTemplate->GetCurrentLODLevelInstance()->GetRequiredModule();

        RequiredModule->Update(Context, DeltaTime);

        TArray<UParticleModule*>& UpdateModules = \
            SpriteTemplate->GetCurrentLODLevelInstance()->GetUpdateModule();

        for (UParticleModule* UpdateModule : UpdateModules)
        {
            if (UpdateModule->GetActive()) 
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
    for (int32 Index = 0; Index < SpawnNum; Index++)
    {
        if (ActiveParticles >= MaxActiveParticles)
        {
            // UE_LOG("[FParticleEmitterInstance::SpawnParticles][Warning] Reached max particle count.");
            break;
        }

        DECLARE_PARTICLE_PTR(ParticlePtr, ParticleData + ParticleStride * ActiveParticles);
        ActiveParticles++;

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
        UParticleModuleRequired* RequiredModule = SpriteTemplate->GetCurrentLODLevelInstance()->GetRequiredModule();
        if (RequiredModule)
        {
            RequiredModule->Spawn(Context, StartTime);
        }

        // 추가 SpawnModules 호출
        for (UParticleModule* Module : SpriteTemplate->GetCurrentLODLevelInstance()->GetSpawnModule())
        {
            if (Module->GetActive()) 
            {
                Module->Spawn(Context, StartTime);
            }
        }

        // TypeDataModule의 Spawn 호출 (Ribbon/Beam 등에서 SpawnTime 설정)
        // Index * Increment로 시간 분산하여 각 파티클이 고유한 SpawnTime을 가짐
        UParticleModuleTypeDataBase* TypeDataModule = SpriteTemplate->GetCurrentLODLevelInstance()->GetTypeDataModule();
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
    return SpriteTemplate->
        GetCurrentLODLevelInstance()->
        GetRequiredModule()->
        GetLifeTime();
}