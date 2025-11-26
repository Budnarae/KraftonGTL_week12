#include "pch.h"
#include "ParticleSystemComponent.h"

#include "ParticleEmitterInstance.h"
#include "Keyboard.h"
#include "ParticleData.h"
#include "ParticleHelper.h"
#include "ParticleInstanceBuffer.h"
#include "ParticleLODLevel.h"
#include "ParticleModuleTypeDataBase.h"
#include "ParticleModuleTypeDataBeam.h"
#include "ParticleModuleTypeDataRibbon.h"
#include "ParticleModuleBeamTarget.h"
#include "ParticleModuleBeamNoise.h"
#include "ParticleModuleBeamWidth.h"
#include "ParticleModuleBeamColorOverLength.h"
#include "ParticleModuleRibbonWidth.h"
#include "ParticleModuleRibbonColorOverLength.h"
#include "ParticleModuleColor.h"
#include "MeshBatchElement.h"
#include "SceneView.h"
#include "ResourceManager.h"
#include "Material.h"
#include "Shader.h"
#include "Quad.h"
#include "Texture.h"
#include "StaticMesh.h"
#include "Actor.h"
#include "D3D11RHI.h"
#include "StatManagement/ParticleStatManager.h"

UParticleSystemComponent::UParticleSystemComponent()
{
    bCanEverTick = true;

    // 에디터에서도 파티클 시뮬레이션이 실행되도록 설정
    // (어떤 액터에 붙어있든 에디터 뷰포트에서 실시간 미리보기 제공)
    bTickInEditor = true;

    // BeamBuffers, RibbonBuffers는 TMap으로 자동 초기화됨
}

UParticleSystemComponent::~UParticleSystemComponent()
{
    Deactivate();

    // 모든 Beam 버퍼 해제
    for (auto& Pair : BeamBuffers)
    {
        delete Pair.second;
    }
    BeamBuffers.clear();

    // 모든 Ribbon 버퍼 해제
    for (auto& Pair : RibbonBuffers)
    {
        delete Pair.second;
    }
    RibbonBuffers.clear();
}

// ============================================================================
// 복사 관련 (Duplication)
// ============================================================================

// ----------------------------------------------------------------------------
// DuplicateSubObjects
// ----------------------------------------------------------------------------
// 얕은 복사된 멤버들에 대해 깊은 복사를 수행합니다.
//
// 처리 순서:
// 1. Super::DuplicateSubObjects() 호출 (상위 클래스 복사 처리)
// 2. Template - 얕은 복사 유지 (공유 Asset)
// 3. EmitterInstances - 깊은 복사 수행
//    - 각 FParticleEmitterInstance를 새로 생성
//    - ParticleData 메모리 블록 복사
//    - OwnerComponent를 새 컴포넌트(this)로 설정
// ----------------------------------------------------------------------------
void UParticleSystemComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    // ------------------------------------------------------------------------
    // EmitterInstances 깊은 복사
    // ------------------------------------------------------------------------
    // 현재 EmitterInstances는 원본의 포인터들을 얕은 복사한 상태입니다.
    // 각 인스턴스를 새로 생성하고 데이터를 복사해야 합니다.

    // Step 1: 원본 포인터 배열을 임시 저장
    TArray<FParticleEmitterInstance*> OldInstances = EmitterInstances;

    // Step 2: 현재 배열을 비움 (메모리 해제 아님 - 원본 것이므로)
    EmitterInstances.Empty();

    // Step 3: 각 인스턴스를 깊은 복사 (복사 생성자 사용)
    for (FParticleEmitterInstance* OldInstance : OldInstances)
    {
        if (!OldInstance)
        {
            continue;
        }

        // 복사 생성자를 사용하여 새 인스턴스 생성
        FParticleEmitterInstance* NewInstance = new FParticleEmitterInstance(*OldInstance);

        // 소유자를 새 컴포넌트(this)로 설정
        NewInstance->OwnerComponent = this;

        // 새 인스턴스를 배열에 추가
        EmitterInstances.Add(NewInstance);
    }

    // ------------------------------------------------------------------------
    // D3D 버퍼는 복사되면 안 됨 - 원본과 공유를 끊고 빈 맵으로 초기화
    // ------------------------------------------------------------------------
    // 얕은 복사로 원본의 포인터가 복사되었으므로, 여기서 클리어하여 원본과 분리
    // (delete 하지 않음 - 원본의 버퍼이므로)
    BeamBuffers.clear();
    RibbonBuffers.clear();
}

// ----------------------------------------------------------------------------
// PostDuplicate
// ----------------------------------------------------------------------------
// 복사가 완료된 후 상태를 초기화합니다.
//
// 복사본은 새로운 인스턴스이므로:
// - ElapsedTime을 0으로 리셋하여 처음부터 재생
// ----------------------------------------------------------------------------
void UParticleSystemComponent::PostDuplicate()
{
    Super::PostDuplicate();

    // 복사본은 처음부터 시작하도록 경과 시간 초기화
    ElapsedTime = 0.0f;

    // D3D 버퍼는 DuplicateSubObjects에서 이미 클리어됨
    // 필요 시 CollectMeshBatches에서 자동 생성됨

    // 거리 기반 스폰 상태 초기화
    AccumulatedDistance = 0.0f;
    bHasPreviousLocation = false;
}

// Getters
UParticleSystem* UParticleSystemComponent::GetTemplate() const
{
    return Template;
}

// bool UParticleSystemComponent::GetIsActive() const
// {
//     return bIsActive;
// }

float UParticleSystemComponent::GetElapsedTime() const
{
    return ElapsedTime;
}

TArray<FParticleEmitterInstance*>& UParticleSystemComponent::GetSystemInstance()
{
    return EmitterInstances;
}

const TArray<FParticleEmitterInstance*>& UParticleSystemComponent::GetSystemInstance() const
{
    return EmitterInstances;
}

int32 UParticleSystemComponent::GetCurrentLODLevel() const
{
    return CurrentLODLevel;
}

// Setters
void UParticleSystemComponent::SetTemplate(UParticleSystem* InTemplate)
{
    Template = InTemplate;
    // 템플릿의 하위 에미터의 LODLevel을 설정한다.
    SetCurrentLODLevel(CurrentLODLevel);
}

// void UParticleSystemComponent::SetIsActive(bool bInIsActive)
// {
//     bIsActive = bInIsActive;
// }

void UParticleSystemComponent::SetElapsedTime(float InElapsedTime)
{
    ElapsedTime = InElapsedTime;
}

void UParticleSystemComponent::SetCurrentLODLevel(const int32 InCurrentLODLevel)
{
    if (InCurrentLODLevel < MIN_PARTICLE_LODLEVEL || InCurrentLODLevel >= MAX_PARTICLE_LODLEVEL)
    {
        UE_LOG("[UParticleSystemComponent::SetCurrentLODLevel][Warning] There's no such LOD Level.");
        return;
    }

    // 같은 LOD 레벨이면 아무 작업도 하지 않음
    if (CurrentLODLevel == InCurrentLODLevel)
    {
        return;
    }

    // 템플릿의 하위 에미터들에도 Current Level을 설정한다.
    if (!Template) return;

    // -------------------------------------------
    // TypeData 일관성 검증 (LOD 간 TypeData가 다르면 경고)
    // -------------------------------------------
    for (UParticleEmitter* ParticleEmitter : Template->GetEmitters())
    {
        if (!ParticleEmitter) continue;

        UParticleLODLevel* OldLODLevel = ParticleEmitter->GetParticleLODLevelWithIndex(CurrentLODLevel);
        UParticleLODLevel* NewLODLevel = ParticleEmitter->GetParticleLODLevelWithIndex(InCurrentLODLevel);

        if (OldLODLevel && NewLODLevel)
        {
            UParticleModuleTypeDataBase* OldTypeData = OldLODLevel->GetTypeDataModule();
            UParticleModuleTypeDataBase* NewTypeData = NewLODLevel->GetTypeDataModule();

            // TypeData 타입이 다르면 경고 (Sprite→Ribbon 등 변경 시 문제 발생)
            EDynamicEmitterType OldType = OldTypeData ? OldTypeData->GetEmitterType() : EDynamicEmitterType::EDET_Sprite;
            EDynamicEmitterType NewType = NewTypeData ? NewTypeData->GetEmitterType() : EDynamicEmitterType::EDET_Sprite;

            if (OldType != NewType)
            {
                UE_LOG("[UParticleSystemComponent::SetCurrentLODLevel][Warning] "
                       "Emitter '%s' has different TypeData between LOD %d (%d) and LOD %d (%d). "
                       "This may cause rendering issues!",
                       ParticleEmitter->GetEmitterName().c_str(),
                       CurrentLODLevel, (int)OldType,
                       InCurrentLODLevel, (int)NewType);
            }
        }
    }

    // -------------------------------------------
    // Step 1: 기존 파티클 모두 제거 (리플레이 준비)
    // 언리얼 Cascade 방식: LOD 전환 시 처음부터 다시 시작
    // -------------------------------------------
    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (Instance)
        {
            Instance->KillAllParticles();
        }
    }

    CurrentLODLevel = InCurrentLODLevel;

    // -------------------------------------------
    // Step 2: 에미터들의 LOD 설정 및 PayloadOffset 재계산
    // -------------------------------------------
    TArray<UParticleEmitter*>& Emitters = Template->GetEmitters();
    for (UParticleEmitter* ParticleEmitter : Emitters)
    {
        ParticleEmitter->SetCurrentLODLevel(CurrentLODLevel);
        // 새 LOD의 모듈 구성에 맞게 PayloadOffset 재계산
        ParticleEmitter->CacheEmitterModuleInfo();
    }

    // -------------------------------------------
    // Step 3: EmitterInstance 메모리 재할당
    // 새 LOD의 ParticleStride에 맞게 메모리 재할당
    // -------------------------------------------
    for (int32 i = 0; i < EmitterInstances.Num() && i < Emitters.Num(); ++i)
    {
        FParticleEmitterInstance* Instance = EmitterInstances[i];
        UParticleEmitter* Emitter = Emitters[i];

        if (!Instance || !Emitter) continue;

        // 기존 메모리 해제
        if (Instance->ParticleData)
        {
            free(Instance->ParticleData);
            Instance->ParticleData = nullptr;
        }

        // 새 LOD 기준으로 ParticleStride 재계산
        int32 NewParticleStride = Emitter->GetRequiredMemorySize();
        Instance->ParticleStride = NewParticleStride;

        // 새 메모리 할당
        if (Instance->MaxActiveParticles > 0 && NewParticleStride > 0)
        {
            Instance->ParticleData = (uint8*)malloc(
                Instance->MaxActiveParticles * NewParticleStride
            );
        }

        // LOD 참조 갱신
        Instance->CurrentLODLevelIndex = CurrentLODLevel;
        Instance->CurrentLODLevel = Emitter->GetCurrentLODLevelInstance();

        // EmitterType 갱신 (TypeData가 다를 수 있으므로)
        UParticleModuleTypeDataBase* TypeDataModule = Instance->CurrentLODLevel ?
            Instance->CurrentLODLevel->GetTypeDataModule() : nullptr;
        if (TypeDataModule)
        {
            Instance->EmitterType = TypeDataModule->GetEmitterType();
        }
        else
        {
            Instance->EmitterType = EDynamicEmitterType::EDET_Sprite;
        }

        // SpawnRate 갱신 (새 LOD의 RequiredModule에서)
        if (Instance->CurrentLODLevel && Instance->CurrentLODLevel->GetRequiredModule())
        {
            Instance->SpawnRate = Instance->CurrentLODLevel->GetRequiredModule()->GetSpawnRate();
        }

        // 스폰 관련 변수 리셋
        Instance->SpawnFraction = 0.0f;
        Instance->SpawnNum = 0;
        Instance->ActiveParticles = 0;
    }

    // -------------------------------------------
    // Step 4: 시뮬레이션 시간 리셋 (처음부터 다시 시작)
    // -------------------------------------------
    ElapsedTime = 0.0f;
    LastLODDistanceCheckTime = 0.0f;

    UE_LOG("[UParticleSystemComponent::SetCurrentLODLevel] LOD changed to %d, replay started.", CurrentLODLevel);
}

// -------------------------------------------
// LOD 거리 기반 전환 (LOD Distance-Based Switching)
// -------------------------------------------

void UParticleSystemComponent::UpdateLODFromDistance(const FVector& CameraLocation)
{
    if (!Template)
    {
        return;
    }

    // 강제 LOD 모드에서는 자동 LOD 전환하지 않음 (에디터에서 LOD 미리보기용)
    if (bOverrideLOD)
    {
        return;
    }

    // DirectSet 모드에서는 자동 LOD 전환하지 않음
    if (Template->GetLODMethod() == EParticleLODMethod::DirectSet)
    {
        return;
    }

    // 파티클 시스템의 월드 위치와 카메라 간의 거리 계산
    FVector ParticleLocation = GetWorldLocation();
    float DistanceToCamera = (CameraLocation - ParticleLocation).Size();

    // 거리에 해당하는 LOD 레벨 결정
    int32 NewLODLevel = Template->GetLODLevelForDistance(DistanceToCamera);

    // LOD 레벨이 변경되었으면 적용
    if (NewLODLevel != CurrentLODLevel)
    {
        SetCurrentLODLevel(NewLODLevel);
    }
}

// SystemInstance 배열 관련 함수
void UParticleSystemComponent::AddEmitterInstance(FParticleEmitterInstance* Instance)
{
    if (!Instance)
    {
        UE_LOG("[UParticleSystemComponent::AddSystemInstance][Warning] Parameter is null.");
        return;
    }

    EmitterInstances.Add(Instance);
}

bool UParticleSystemComponent::RemoveEmitterInstance(FParticleEmitterInstance* Instance)
{
    FParticleEmitterInstance* tmp = Instance;
    bool result = EmitterInstances.Remove(Instance);
    delete tmp;
    return result;
}

bool UParticleSystemComponent::RemoveEmitterInstanceAt(int32 Index)
{
    if (Index < 0 || Index >= EmitterInstances.Num())
    {
        UE_LOG("[UParticleSystemComponent::RemoveEmitterInstanceAt][Warning] Index out of range.");
        return false;
    }

    FParticleEmitterInstance* tmp = EmitterInstances[Index];
    EmitterInstances.RemoveAt(Index);
    delete tmp;
    return true;
}

void UParticleSystemComponent::AddCollisionEvent(const FParticleEventCollideData& CollideEvent)
{
    CollisionEvents.Add(CollideEvent);
}

void UParticleSystemComponent::ClearCollisionEvents()
{
    CollisionEvents.Empty();
}

void UParticleSystemComponent::AddDeathEvent(const FParticleEventDeathData& DeathEvent)
{
    DeathEvents.Add(DeathEvent);
}

void UParticleSystemComponent::ClearDeathEvents()
{
    DeathEvents.Empty();
}

void UParticleSystemComponent::AddSpawnEvent(const FParticleEventSpawnData& SpawnEvent)
{
    SpawnEvents.Add(SpawnEvent);
}

void UParticleSystemComponent::ClearSpawnEvents()
{
    SpawnEvents.Empty();
}

void UParticleSystemComponent::ClearAllEvents()
{
    ClearCollisionEvents();
    ClearDeathEvents();
    ClearSpawnEvents();
}

// 시뮬레이션 시작 명령 (파티클 생성 및 타이머 시작)
void UParticleSystemComponent::Activate(bool bReset)
{
    if (!Template)
    {
        UE_LOG("[UParticleSystemComponent::Activate][Warning] There's no particle template.");
        return;
    }

    if (!Template->IsValid())
    {
        UE_LOG("[UParticleSystemComponent::Activate][Warning] Particle template isn't valid.");
        return;
    }

    // -------------------------------------------
    // LOD 초기화 (LODMethod에 따른 분기)
    // -------------------------------------------
    EParticleLODMethod LODMethod = Template->GetLODMethod();

    if (LODMethod == EParticleLODMethod::Automatic || LODMethod == EParticleLODMethod::ActivateAutomatic)
    {
        // 카메라 위치가 캐시되어 있으면 거리 기반 LOD 설정
        if (bHasCachedCameraLocation)
        {
            UpdateLODFromDistance(CachedCameraLocation);
        }
        else if (!bOverrideLOD)
        {
            // 카메라 위치가 없으면 기본값 LOD 0
            // bOverrideLOD가 true면 현재 설정된 LOD 레벨 유지 (에디터 LOD 미리보기용)
            SetCurrentLODLevel(0);
        }
    }
    else // DirectSet
    {
        // DirectSet 모드: 기존 LOD 레벨 유지 (처음이면 0)
        if (CurrentLODLevel < MIN_PARTICLE_LODLEVEL || CurrentLODLevel >= MAX_PARTICLE_LODLEVEL)
        {
            SetCurrentLODLevel(0);
        }
    }

    // LOD 거리 체크 타이머 초기화
    LastLODDistanceCheckTime = 0.0f;
    for (FParticleEmitterInstance* EmitterInstance : EmitterInstances)
    {
        delete EmitterInstance;
    }
    EmitterInstances.Empty();

    for (UParticleEmitter* Emitter : Template->GetEmitters())
    {
        if (Emitter->GetActive() == false)
        {
            continue;
        }

        // 페이로드 관련 정보 초기화
        Emitter->CacheEmitterModuleInfo();
        
        FParticleEmitterInstance* Instance = new FParticleEmitterInstance;

        // 템플릿 및 소유자 참조
        Instance->SpriteTemplate = Emitter;
        Instance->OwnerComponent = this;

        // LOD 및 모듈 참조
        Instance->CurrentLODLevelIndex = CurrentLODLevel;
        Instance->CurrentLODLevel = Emitter->GetCurrentLODLevelInstance();

        // EmitterType 설정 (TypeDataModule에서 가져옴)
        UParticleModuleTypeDataBase* TypeDataModule = Instance->CurrentLODLevel ?
            Instance->CurrentLODLevel->GetTypeDataModule() : nullptr;
        if (TypeDataModule)
        {
            Instance->EmitterType = TypeDataModule->GetEmitterType();
        }
        else
        {
            Instance->EmitterType = EDET_Sprite;  // TypeDataModule이 없으면 기본 Sprite
        }

        // 메모리 관리 및 상태 변수
        Instance->MaxActiveParticles = Emitter->GetMaxParticleCount();
        Instance->ParticleStride = Emitter->GetRequiredMemorySize();
        Instance->ParticleData = (uint8*)malloc(
            Instance->MaxActiveParticles * Instance->ParticleStride
        );

        // 런타입 파티클 생성 관리
        Instance->SpawnRate =
            Emitter->GetCurrentLODLevelInstance()->GetRequiredModule()->GetSpawnRate();
        Instance->Duration = Emitter->GetCalculatedDuration();
        
        EmitterInstances.Add(Instance);
    }
}

// 시뮬레이션 종료 명령
void UParticleSystemComponent::Deactivate()
{
    // Dynamic Data 해제
    ReleaseDynamicData();

    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (Instance)
        {
            if (Instance->ParticleData)
            {
                free(Instance->ParticleData);
                Instance->ParticleData = nullptr;
            }
            delete Instance;
        }
    }
    EmitterInstances.clear();
}

// Dynamic Data 생성
void UParticleSystemComponent::CreateDynamicData()
{
    // 기존 Dynamic Data 해제
    ReleaseDynamicData();

    // 각 EmitterInstance에 대해 Dynamic Data 생성
    for (int32 i = 0; i < EmitterInstances.Num(); ++i)
    {
        FParticleEmitterInstance* Instance = EmitterInstances[i];
        if (!Instance || Instance->ActiveParticles == 0)
        {
            continue;
        }

        // EmitterType에 따라 다른 DynamicData 클래스 생성
        FDynamicEmitterRenderData* DynamicData = nullptr;

        switch (Instance->EmitterType)
        {
        case EDET_Sprite:
        {
            FDynamicSpriteEmitterData* SpriteData = new FDynamicSpriteEmitterData();
            SpriteData->Init(Instance, i);
            DynamicData = SpriteData;
            break;
        }
        case EDET_Mesh:
        {
            FDynamicMeshEmitterData* MeshData = new FDynamicMeshEmitterData();
            MeshData->Init(Instance, i);
            DynamicData = MeshData;
            break;
        }
        case EDET_Beam:
        case EDET_Ribbon:
        {
            // Beam/Ribbon은 CollectMeshBatches에서 직접 렌더링하므로
            // DynamicData 생성하지 않고 스킵
            continue;
        }
        default:
            UE_LOG("[CreateDynamicData][Warning] Unsupported emitter type: %d", (int)Instance->EmitterType);
            continue;
        }

        if (DynamicData)
        {
            DynamicEmitterData.Add(DynamicData);
        }
    }
}

// Dynamic Data 해제
void UParticleSystemComponent::ReleaseDynamicData()
{
    for (FDynamicEmitterRenderData* DynamicData : DynamicEmitterData)
    {
        if (DynamicData)
        {
            delete DynamicData;  // 다형성으로 인해 올바른 소멸자 호출됨
        }
    }
    DynamicEmitterData.clear();
}

// [Tick Phase] 매 프레임 호출되어 DeltaTime만큼 시뮬레이션을 전진시킵니다. (가장 중요)
void UParticleSystemComponent::TickComponent(float DeltaTime)
{
    // 임시 상수
    const static FVector Velocity = FVector(0, 0, 0.1f);

    // 이전 틱의 collision events 클리어
    ClearCollisionEvents();

    ElapsedTime += DeltaTime;

    // -------------------------------------------
    // LOD 거리 체크 (Automatic 모드)
    // bOverrideLOD가 true이면 자동 LOD 전환 건너뜀 (에디터에서 직접 LOD 지정)
    // -------------------------------------------
    if (Template && Template->GetLODMethod() == EParticleLODMethod::Automatic && !bOverrideLOD)
    {
        float LODCheckInterval = Template->GetLODDistanceCheckTime();

        // 주기적으로 LOD 거리 체크 수행
        if (ElapsedTime - LastLODDistanceCheckTime >= LODCheckInterval)
        {
            LastLODDistanceCheckTime = ElapsedTime;

            if (bHasCachedCameraLocation)
            {
                UpdateLODFromDistance(CachedCameraLocation);
            }
        }
    }

    // 현재 위치 저장
    FVector CurrentLocation = GetWorldLocation();

    // 이전 위치 초기화 (첫 프레임)
    if (!bHasPreviousLocation)
    {
        PreviousWorldLocation = CurrentLocation;
        bHasPreviousLocation = true;
    }

    // 이동 거리 계산
    float MovedDistance = (CurrentLocation - PreviousWorldLocation).Size();

    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (Instance)
        {
            // 기존 파티클 업데이트 (수명 체크 등)
            Instance->Update(DeltaTime);

            // 스폰 수 결정: 거리 기반 vs 시간 기반
            if (DistancePerSpawn > 0.0f)
            {
                // 거리 기반 스폰
                AccumulatedDistance += MovedDistance;
                int32 SpawnsNeeded = static_cast<int32>(AccumulatedDistance / DistancePerSpawn);
                AccumulatedDistance -= SpawnsNeeded * DistancePerSpawn;
                Instance->SpawnNum = SpawnsNeeded;
            }
            // else: 시간 기반은 Update()에서 이미 SpawnNum 계산됨

            // Increment 계산 (파티클들이 시간/공간적으로 균등 분산)
            float Increment = (Instance->SpawnNum > 0) ? (DeltaTime / Instance->SpawnNum) : 0.0f;

            // 이전 위치와 현재 위치를 전달하여 파티클 위치 보간
            Instance->SpawnParticles(
                ElapsedTime - DeltaTime,  // 이번 프레임의 시작 시간
                Increment,
                PreviousWorldLocation,    // 이전 프레임 위치
                CurrentLocation,          // 현재 프레임 위치
                Velocity,
                nullptr
            );
        }
    }

    // 이전 위치 업데이트
    PreviousWorldLocation = CurrentLocation;

    // 3. 시뮬레이션 완료 후 렌더링용 Dynamic Data 생성
    CreateDynamicData();
}

// 렌더링 관련 함수
void UParticleSystemComponent::CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View)
{
    // 1. 유효성 검사
    if (!Template || EmitterInstances.empty())
    {
        return;
    }

    // RHI 유효성 검사 (종료 시 크래시 방지)
    D3D11RHI* RHIDevice = GEngine.GetRHIDevice();
    if (!RHIDevice || !RHIDevice->GetDevice() || !RHIDevice->GetDeviceContext())
    {
        return;
    }

    // ========================================================================
    // 2. Beam/Ribbon 에미터 렌더링 (EmitterInstances 직접 순회)
    // ========================================================================
    for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); ++EmitterIndex)
    {
        FParticleEmitterInstance* Instance = EmitterInstances[EmitterIndex];
        if (!Instance || !Instance->SpriteTemplate)
        {
            continue;
        }

        // TypeDataModule 체크
        UParticleLODLevel* LODLevel = Instance->SpriteTemplate->GetCurrentLODLevelInstance();
        UParticleModuleTypeDataBase* TypeDataModule = LODLevel ? LODLevel->GetTypeDataModule() : nullptr;

        if (!TypeDataModule)
        {
            continue;  // Sprite/Mesh는 아래에서 DynamicEmitterData로 처리
        }

        EDynamicEmitterType EmitterType = TypeDataModule->GetEmitterType();

        // 렌더링에 필요한 리소스 준비
        UParticleModuleRequired* RequiredModule = LODLevel->GetRequiredModule();
        if (!RequiredModule)
        {
            continue;
        }

        UMaterialInterface* ParticleMaterial = RequiredModule->GetMaterial();
        if (!ParticleMaterial)
        {
            continue;
        }

        // Beam/Ribbon은 전용 셰이더(Beam.hlsl)를 사용해야 함
        // RequiredModule의 머티리얼 셰이더(UberLit 등)는 InputLayout이 다르므로 사용 불가
        // 머티리얼은 텍스처 정보만 사용
        UShader* ShaderToUse = nullptr;
        if (EmitterType == EDynamicEmitterType::EDET_Beam || EmitterType == EDynamicEmitterType::EDET_Ribbon)
        {
            // Beam/Ribbon 전용 셰이더 사용 (FBillboardVertex InputLayout과 호환)
            ShaderToUse = UResourceManager::GetInstance().Load<UShader>("Shaders/Effects/Beam.hlsl");
        }
        else
        {
            ShaderToUse = ParticleMaterial->GetShader();
        }

        if (!ShaderToUse)
        {
            continue;
        }

        FShaderVariant* ShaderVariant = ShaderToUse->GetOrCompileShaderVariant(ParticleMaterial->GetShaderMacros());
        if (!ShaderVariant)
        {
            continue;
        }

        // Beam 타입 렌더링
        if (EmitterType == EDynamicEmitterType::EDET_Beam)
        {
            UParticleModuleTypeDataBeam* BeamModule = static_cast<UParticleModuleTypeDataBeam*>(TypeDataModule);

            // 빔 타겟 위치 결정 (우선순위: BeamTarget 모듈 > BeamTargetActor > TypeData 기본값)
            // BeamTarget 모듈이 있으면 자동으로 Target 방식 사용
            UParticleModuleBeamTarget* TargetModule = LODLevel->FindModule<UParticleModuleBeamTarget>();
            bool bUseTargetMethod = (BeamModule->GetBeamMethod() == EBeamMethod::Target) || (TargetModule != nullptr);

            if (bUseTargetMethod)
            {
                FVector ResolvedTarget;
                bool bTargetResolved = false;

                // 1순위: BeamTarget 모듈 확인
                if (TargetModule)
                {
                    bTargetResolved = TargetModule->ResolveTargetPoint(this, ResolvedTarget);
                }

                // 2순위: Component의 BeamTargetActor
                if (!bTargetResolved && BeamTargetActor)
                {
                    ResolvedTarget = BeamTargetActor->GetActorLocation();
                    bTargetResolved = true;
                }

                // 타겟이 결정되었으면 적용
                if (bTargetResolved)
                {
                    // BeamMethod를 Target으로 설정하고 타겟 포인트 적용
                    BeamModule->SetBeamMethod(EBeamMethod::Target);
                    BeamModule->SetTargetPoint(ResolvedTarget);
                }
            }

            // 빔 소스 위치 (기존 로직 유지 - 추후 BeamSource 모듈로 분리 가능)
            if (BeamSourceActor)
            {
                BeamModule->SetSourcePoint(BeamSourceActor->GetActorLocation() - GetWorldLocation());
            }

            // 빔 노이즈 파라미터 준비 (모듈이 있으면 가져옴)
            const FBeamNoiseParams* NoiseParams = nullptr;
            FBeamNoiseParams NoiseParamsStorage;

            if (UParticleModuleBeamNoise* NoiseModule = LODLevel->FindModule<UParticleModuleBeamNoise>())
            {
                if (NoiseModule->GetActive())
                {
                    NoiseParamsStorage = NoiseModule->GetParams();
                    NoiseParams = &NoiseParamsStorage;
                }
            }

            // 빔 너비 파라미터 준비 (모듈이 있으면 가져옴)
            const FBeamWidthParams* WidthParams = nullptr;
            FBeamWidthParams WidthParamsStorage;

            if (UParticleModuleBeamWidth* WidthModule = LODLevel->FindModule<UParticleModuleBeamWidth>())
            {
                if (WidthModule->GetActive())
                {
                    WidthParamsStorage = WidthModule->GetParams();
                    WidthParams = &WidthParamsStorage;
                }
            }

            // 빔 컬러 파라미터 준비 (Color 모듈 우선, 없으면 TypeDataBeam의 BeamColor)
            UParticleModuleColor* ColorModule = LODLevel->FindModule<UParticleModuleColor>();

            // 기본 색상: TypeDataBeam의 BeamColor (흰색 기본)
            FLinearColor BaseBeamColor(1.0f, 1.0f, 1.0f, 1.0f);
            if (ColorModule)
            {
                // Color 모듈이 있으면 초기 색상 사용
                FVector ColorVec = ColorModule->GetStartColor().GetValue(ElapsedTime);
                float Alpha = ColorModule->GetStartAlpha().GetValue(ElapsedTime);
                if (ColorModule->GetClampAlpha())
                    Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
                BaseBeamColor = FLinearColor(ColorVec.X, ColorVec.Y, ColorVec.Z, Alpha);
            }
            else
            {
                // Color 모듈이 없으면 TypeDataBeam의 BeamColor 사용
                FVector4 TypeDataColor = BeamModule->GetBeamColor();
                BaseBeamColor = FLinearColor(TypeDataColor.X, TypeDataColor.Y, TypeDataColor.Z, TypeDataColor.W);
            }

            // 빔 길이별 색상 모듈 (공간 기반 그라데이션)
            UParticleModuleBeamColorOverLength* ColorOverLengthModule = LODLevel->FindModule<UParticleModuleBeamColorOverLength>();
            FBeamColorParams ColorOverLengthParams;
            bool bHasColorOverLength = (ColorOverLengthModule && ColorOverLengthModule->GetActive());
            if (bHasColorOverLength)
            {
                ColorOverLengthParams = ColorOverLengthModule->GetParams();
            }

            // 빔 포인트 계산 (nullptr이면 해당 기능 비활성화)
            TArray<FVector> BeamPoints;
            TArray<float> BeamWidths;
            FMatrix EmitterRotation = GetWorldTransform().ToRotationScaleMatrix();
            BeamModule->CalculateBeamPoints(GetWorldLocation(), EmitterRotation, BeamPoints, BeamWidths, ElapsedTime, NoiseParams, WidthParams);

            if (BeamPoints.Num() < 2)
                continue;

            // 각 포인트에 대해 Right 벡터 계산
            TArray<FVector> PointRightVectors;
            PointRightVectors.SetNum(BeamPoints.Num());

            for (int32 i = 0; i < BeamPoints.Num(); ++i)
            {
                FVector Forward = FVector::Zero();

                if (i > 0)
                    Forward += (BeamPoints[i] - BeamPoints[i - 1]).GetNormalized();
                if (i < BeamPoints.Num() - 1)
                    Forward += (BeamPoints[i + 1] - BeamPoints[i]).GetNormalized();

                if (Forward.Size() < 0.001f)
                    Forward = FVector(1, 0, 0);
                else
                    Forward = Forward.GetNormalized();

                FVector ToCamera = (View->ViewLocation - BeamPoints[i]).GetNormalized();
                FVector Right = FVector::Cross(Forward, ToCamera);

                if (Right.Size() < 0.001f)
                {
                    Right = FVector::Cross(Forward, FVector(0, 0, 1));
                    if (Right.Size() < 0.001f)
                        Right = FVector::Cross(Forward, FVector(0, 1, 0));
                }

                PointRightVectors[i] = Right.GetNormalized();
            }

            // 정점 데이터 생성
            uint32 NumPoints = static_cast<uint32>(BeamPoints.Num());
            uint32 NumVertices = NumPoints * 2;
            uint32 NumSegments = NumPoints - 1;
            uint32 NumIndices = NumSegments * 6;

            TArray<FBillboardVertex> Vertices;
            TArray<uint32> Indices;
            Vertices.SetNum(NumVertices);
            Indices.SetNum(NumIndices);

            for (uint32 i = 0; i < NumPoints; ++i)
            {
                FVector Point = BeamPoints[i];
                FVector Right = PointRightVectors[i];
                float Width = BeamWidths[i];
                float T = static_cast<float>(i) / static_cast<float>(NumPoints - 1);  // 0=시작, 1=끝

                // 정점 색상 = BaseBeamColor (Color 모듈 또는 TypeDataBeam의 BeamColor)
                FLinearColor VertexColor = BaseBeamColor;

                // ColorOverLength 모듈이 있으면 위치에 따른 그라데이션 적용 (곱연산)
                if (bHasColorOverLength)
                {
                    FLinearColor LengthColor = ColorOverLengthParams.EvaluateAtT(T);
                    VertexColor.R *= LengthColor.R;
                    VertexColor.G *= LengthColor.G;
                    VertexColor.B *= LengthColor.B;
                    VertexColor.A *= LengthColor.A;
                }

                Vertices[i * 2].WorldPosition = Point - Right * Width * 0.5f;
                Vertices[i * 2].UV = FVector2D(T, 0.0f);
                Vertices[i * 2].Color = VertexColor;

                Vertices[i * 2 + 1].WorldPosition = Point + Right * Width * 0.5f;
                Vertices[i * 2 + 1].UV = FVector2D(T, 1.0f);
                Vertices[i * 2 + 1].Color = VertexColor;
            }

            for (uint32 i = 0; i < NumSegments; ++i)
            {
                uint32 BaseIndex = i * 6;
                uint32 V0 = i * 2;
                uint32 V1 = i * 2 + 1;
                uint32 V2 = i * 2 + 2;
                uint32 V3 = i * 2 + 3;

                Indices[BaseIndex + 0] = V0;
                Indices[BaseIndex + 1] = V1;
                Indices[BaseIndex + 2] = V2;
                Indices[BaseIndex + 3] = V1;
                Indices[BaseIndex + 4] = V3;
                Indices[BaseIndex + 5] = V2;
            }

            // 에미터별 버퍼 가져오기 (없으면 생성)
            FDynamicMeshBuffer*& BeamBuffer = BeamBuffers[EmitterIndex];
            if (!BeamBuffer)
            {
                BeamBuffer = new FDynamicMeshBuffer();
            }

            // 동적 버퍼 업데이트
            if (!BeamBuffer->Update(Vertices, Indices))
            {
                UE_LOG("[UParticleSystemComponent::CollectMeshBatches][Warning] Failed to update Beam buffer for emitter %d.", EmitterIndex);
                continue;
            }

            // BatchElement 생성
            FMeshBatchElement BatchElement;
            BatchElement.VertexShader = ShaderVariant->VertexShader;
            BatchElement.PixelShader = ShaderVariant->PixelShader;
            BatchElement.InputLayout = ShaderVariant->InputLayout;
            BatchElement.Material = ParticleMaterial;
            BatchElement.VertexBuffer = BeamBuffer->GetVertexBuffer();
            BatchElement.IndexBuffer = BeamBuffer->GetIndexBuffer();
            BatchElement.VertexStride = BeamBuffer->GetVertexStride();
            BatchElement.IndexCount = BeamBuffer->GetIndexCount();
            BatchElement.StartIndex = 0;
            BatchElement.BaseVertexIndex = 0;
            BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            BatchElement.WorldMatrix = FMatrix::Identity();

            // 정점 색상을 사용하므로 InstanceColor는 GlowIntensity만 적용
            // (정점 색상이 이미 Color/ColorOverLife 모듈 값을 반영함)
            float GlowIntensity = BeamModule->GetGlowIntensity();
            BatchElement.InstanceColor = FLinearColor(
                GlowIntensity,
                GlowIntensity,
                GlowIntensity,
                1.0f
            );

            BatchElement.UVStart = 0.0f;
            BatchElement.UVEnd = 1.0f;

            bool bUseTexture = BeamModule->GetUseTexture();
            BatchElement.UseTexture = bUseTexture ? 1.0f : 0.0f;

            if (bUseTexture)
            {
                // BeamModule에 직접 설정된 텍스처 우선 사용
                if (UTexture* DirectTexture = BeamModule->GetBeamTexture())
                {
                    BatchElement.InstanceShaderResourceView = DirectTexture->GetShaderResourceView();
                }
                // 없으면 머티리얼에서 가져옴
                else if (ParticleMaterial)
                {
                    if (UTexture* TextureData = ParticleMaterial->GetTexture(EMaterialTextureSlot::Diffuse))
                    {
                        BatchElement.InstanceShaderResourceView = TextureData->GetShaderResourceView();
                    }
                    else
                    {
                        BatchElement.InstanceShaderResourceView = nullptr;
                    }
                }
                else
                {
                    BatchElement.InstanceShaderResourceView = nullptr;
                }
            }
            else
            {
                BatchElement.InstanceShaderResourceView = nullptr;
            }

            BatchElement.ObjectID = InternalIndex;
            OutMeshBatchElements.Add(BatchElement);
            continue;
        }

        // Ribbon 타입 렌더링
        if (EmitterType == EDynamicEmitterType::EDET_Ribbon)
        {
            UParticleModuleTypeDataRibbon* RibbonModule = static_cast<UParticleModuleTypeDataRibbon*>(TypeDataModule);

            // 리본 너비 파라미터 준비 (모듈이 있으면 가져옴)
            const FRibbonWidthParams* WidthParams = nullptr;
            FRibbonWidthParams WidthParamsStorage;

            if (UParticleModuleRibbonWidth* WidthModule = LODLevel->FindModule<UParticleModuleRibbonWidth>())
            {
                if (WidthModule->GetActive())
                {
                    WidthParamsStorage = WidthModule->GetParams();
                    WidthParams = &WidthParamsStorage;
                }
            }

            // 리본 길이별 색상 파라미터 준비 (모듈이 있으면 가져옴)
            const FRibbonColorParams* ColorParams = nullptr;
            FRibbonColorParams ColorParamsStorage;

            if (UParticleModuleRibbonColorOverLength* ColorModule = LODLevel->FindModule<UParticleModuleRibbonColorOverLength>())
            {
                if (ColorModule->GetActive())
                {
                    ColorParamsStorage = ColorModule->GetParams();
                    ColorParams = &ColorParamsStorage;
                }
            }

            TArray<FVector> RibbonPoints;
            TArray<float> RibbonWidths;
            TArray<FLinearColor> RibbonColors;
            RibbonModule->BuildRibbonFromParticles(Instance, GetWorldLocation(), RibbonPoints, RibbonWidths, RibbonColors, WidthParams, ColorParams);

            if (RibbonPoints.Num() < 2)
                continue;

            TArray<FVector> PointRightVectors;
            PointRightVectors.SetNum(RibbonPoints.Num());

            for (int32 i = 0; i < RibbonPoints.Num(); ++i)
            {
                FVector Forward;
                if (i == 0)
                    Forward = RibbonPoints[1] - RibbonPoints[0];
                else if (i == RibbonPoints.Num() - 1)
                    Forward = RibbonPoints[i] - RibbonPoints[i - 1];
                else
                    Forward = RibbonPoints[i + 1] - RibbonPoints[i - 1];

                Forward = Forward.GetNormalized();

                FVector ToCamera = (View->ViewLocation - RibbonPoints[i]).GetNormalized();
                FVector Right = FVector::Cross(Forward, ToCamera);

                if (Right.Size() < 0.001f)
                {
                    Right = FVector::Cross(Forward, FVector(0, 0, 1));
                    if (Right.Size() < 0.001f)
                        Right = FVector::Cross(Forward, FVector(0, 1, 0));
                }

                PointRightVectors[i] = Right.GetNormalized();
            }

            uint32 NumPoints = static_cast<uint32>(RibbonPoints.Num());
            uint32 NumVertices = NumPoints * 2;
            uint32 NumSegments = NumPoints - 1;
            uint32 NumIndices = NumSegments * 6;

            TArray<FBillboardVertex> Vertices;
            TArray<uint32> Indices;
            Vertices.SetNum(NumVertices);
            Indices.SetNum(NumIndices);

            float TextureRepeat = RibbonModule->GetTextureRepeat();

            for (uint32 i = 0; i < NumPoints; ++i)
            {
                FVector Point = RibbonPoints[i];
                FVector Right = PointRightVectors[i];
                float Width = RibbonWidths[i];
                float U = static_cast<float>(i) / static_cast<float>(NumPoints - 1) * TextureRepeat;
                FLinearColor VertexColor = RibbonColors[i];

                Vertices[i * 2].WorldPosition = Point - Right * Width * 0.5f;
                Vertices[i * 2].UV = FVector2D(U, 0.0f);
                Vertices[i * 2].Color = VertexColor;

                Vertices[i * 2 + 1].WorldPosition = Point + Right * Width * 0.5f;
                Vertices[i * 2 + 1].UV = FVector2D(U, 1.0f);
                Vertices[i * 2 + 1].Color = VertexColor;
            }

            for (uint32 i = 0; i < NumSegments; ++i)
            {
                uint32 BaseIndex = i * 6;
                uint32 V0 = i * 2;
                uint32 V1 = i * 2 + 1;
                uint32 V2 = i * 2 + 2;
                uint32 V3 = i * 2 + 3;

                Indices[BaseIndex + 0] = V0;
                Indices[BaseIndex + 1] = V1;
                Indices[BaseIndex + 2] = V2;
                Indices[BaseIndex + 3] = V1;
                Indices[BaseIndex + 4] = V3;
                Indices[BaseIndex + 5] = V2;
            }

            // 에미터별 버퍼 가져오기 (없으면 생성)
            FDynamicMeshBuffer*& RibbonBuffer = RibbonBuffers[EmitterIndex];
            if (!RibbonBuffer)
            {
                RibbonBuffer = new FDynamicMeshBuffer();
            }

            // 동적 버퍼 업데이트
            if (!RibbonBuffer->Update(Vertices, Indices))
            {
                UE_LOG("[UParticleSystemComponent::CollectMeshBatches][Warning] Failed to update Ribbon buffer for emitter %d.", EmitterIndex);
                continue;
            }

            FMeshBatchElement BatchElement;
            BatchElement.VertexShader = ShaderVariant->VertexShader;
            BatchElement.PixelShader = ShaderVariant->PixelShader;
            BatchElement.InputLayout = ShaderVariant->InputLayout;
            BatchElement.Material = ParticleMaterial;
            BatchElement.VertexBuffer = RibbonBuffer->GetVertexBuffer();
            BatchElement.IndexBuffer = RibbonBuffer->GetIndexBuffer();
            BatchElement.VertexStride = RibbonBuffer->GetVertexStride();
            BatchElement.IndexCount = RibbonBuffer->GetIndexCount();
            BatchElement.StartIndex = 0;
            BatchElement.BaseVertexIndex = 0;
            BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            BatchElement.WorldMatrix = FMatrix::Identity();
            BatchElement.InstanceColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
            BatchElement.UVStart = 0.0f;
            BatchElement.UVEnd = 1.0f;

            bool bUseTexture = RibbonModule->GetUseTexture();
            BatchElement.UseTexture = bUseTexture ? 1.0f : 0.0f;

            if (bUseTexture)
            {
                // RibbonModule에 직접 설정된 텍스처 우선 사용
                if (UTexture* DirectTexture = RibbonModule->GetRibbonTexture())
                {
                    BatchElement.InstanceShaderResourceView = DirectTexture->GetShaderResourceView();
                }
                // 없으면 머티리얼에서 가져옴
                else if (ParticleMaterial)
                {
                    if (UTexture* TextureData = ParticleMaterial->GetTexture(EMaterialTextureSlot::Diffuse))
                    {
                        BatchElement.InstanceShaderResourceView = TextureData->GetShaderResourceView();
                    }
                    else
                    {
                        BatchElement.InstanceShaderResourceView = nullptr;
                    }
                }
                else
                {
                    BatchElement.InstanceShaderResourceView = nullptr;
                }
            }
            else
            {
                BatchElement.InstanceShaderResourceView = nullptr;
            }

            BatchElement.ObjectID = InternalIndex;
            OutMeshBatchElements.Add(BatchElement);
            continue;
        }
    }

    // ========================================================================
    // 3. Sprite/Mesh 에미터 렌더링 (DynamicEmitterData 사용)
    // ========================================================================
    if (DynamicEmitterData.empty())
    {
        return;
    }

    // 타입별로 데이터 수집 (Sprite / Mesh 분리)
    TArray<FParticleInstanceData> SpriteInstanceData;
    TArray<FParticleInstanceData> MeshInstanceData;
    UMaterialInterface* SpriteMaterial = nullptr;
    UMaterialInterface* MeshMaterial = nullptr;
    UStaticMesh* MeshToRender = nullptr;

    // 컴포넌트 위치 캐싱
    const FVector ComponentLocation = GetWorldLocation();

    for (FDynamicEmitterRenderData* DynamicData : DynamicEmitterData)
    {
        if (!DynamicData)
        {
            UE_LOG("[UParticleSystemComponent::CollectMeshBatches][Warning] Shader compilation failed.");
            continue;
        }

        const FDynamicEmitterReplayDataBase& Source = DynamicData->GetSource();
        if (Source.ActiveParticleCount == 0)
        {
            continue;
        }

        // 카메라 기준 파티클 정렬 (Back-to-Front for transparency)
        if (View)
        {
            FDynamicSpriteEmitterDataBase* SpriteDataBase = dynamic_cast<FDynamicSpriteEmitterDataBase*>(DynamicData);
            if (SpriteDataBase)
            {
                SpriteDataBase->SortParticles(View->ViewLocation);
            }
        }

        // 타입별 분기
        if (Source.eEmitterType == EDET_Sprite)
        {
            // 통계 수집: 스프라이트 에미터 카운트
            FParticleStatManager::GetInstance().IncrementSpriteEmitterCount();
            FParticleStatManager::GetInstance().AddTotalEmitterCount(1);

            // 스프라이트 머티리얼 저장 (캐스팅 필요)
            const FDynamicSpriteEmitterReplayDataBase& SpriteSource = static_cast<const FDynamicSpriteEmitterReplayDataBase&>(Source);
            if (!SpriteMaterial && SpriteSource.MaterialInterface)
            {
                SpriteMaterial = SpriteSource.MaterialInterface;
            }

            // RequiredModule에서 CameraFacing 옵션 가져오기
            bool bEnableCameraFacing = true; // 스프라이트 기본값
            if (SpriteSource.RequiredModule)
            {
                UParticleModuleRequired* RequiredModule = reinterpret_cast<UParticleModuleRequired*>(SpriteSource.RequiredModule);
                bEnableCameraFacing = RequiredModule->GetEnableCameraFacing();
            }

            // 스프라이트 인스턴스 데이터 수집
            const uint16* ParticleIndices = Source.DataContainer.ParticleIndices;
            const uint8* ParticleData = Source.DataContainer.ParticleData;
            const int32 ParticleStride = Source.ParticleStride;

            for (int32 i = 0; i < Source.ActiveParticleCount; ++i)
            {
                int32 ParticleIndex = ParticleIndices ? ParticleIndices[i] : i;
                DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndex);

                FParticleInstanceData InstanceData;
                InstanceData.FillFromParticle(Particle, ComponentLocation, bEnableCameraFacing);
                SpriteInstanceData.Add(InstanceData);
            }

            // 통계 수집: 스프라이트 파티클 카운트
            FParticleStatManager::GetInstance().AddSpriteParticleCount(Source.ActiveParticleCount);
        }
        else if (Source.eEmitterType == EDET_Mesh)
        {
            // 통계 수집: 메시 에미터 카운트
            FParticleStatManager::GetInstance().IncrementMeshEmitterCount();
            FParticleStatManager::GetInstance().AddTotalEmitterCount(1);

            // 메시 에미터 데이터 캐스팅
            FDynamicMeshEmitterData* MeshEmitterData = static_cast<FDynamicMeshEmitterData*>(DynamicData);
            const FDynamicMeshEmitterReplayDataBase& MeshSource = static_cast<const FDynamicMeshEmitterReplayDataBase&>(Source);

            // 메시 저장
            if (!MeshToRender && MeshEmitterData->StaticMesh)
            {
                MeshToRender = MeshEmitterData->StaticMesh;
            }

            // 메시의 머티리얼 사용 (RequiredModule에서 가져옴 - DynamicData에 이미 복사됨)
            if (!MeshMaterial && MeshSource.MaterialInterface)
            {
                MeshMaterial = MeshSource.MaterialInterface;
            }

            // RequiredModule에서 CameraFacing 옵션 가져오기
            bool bEnableCameraFacing = false; // 메시 기본값
            if (MeshSource.RequiredModule)
            {
                UParticleModuleRequired* RequiredModule = reinterpret_cast<UParticleModuleRequired*>(MeshSource.RequiredModule);
                bEnableCameraFacing = RequiredModule->GetEnableCameraFacing();
            }

            // 메시 인스턴스 데이터 수집
            const uint16* ParticleIndices = Source.DataContainer.ParticleIndices;
            const uint8* ParticleData = Source.DataContainer.ParticleData;
            const int32 ParticleStride = Source.ParticleStride;

            for (int32 i = 0; i < Source.ActiveParticleCount; ++i)
            {
                int32 ParticleIndex = ParticleIndices ? ParticleIndices[i] : i;
                DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndex);

                FParticleInstanceData InstanceData;
                InstanceData.FillFromParticle(Particle, ComponentLocation, bEnableCameraFacing);
                MeshInstanceData.Add(InstanceData);
            }

            // 통계 수집: 메시 파티클 카운트
            FParticleStatManager::GetInstance().AddMeshParticleCount(Source.ActiveParticleCount);
        }
        else if (Source.eEmitterType == EDET_Beam)
        {
            // 통계 수집: 빔 에미터 카운트
            FParticleStatManager::GetInstance().IncrementBeamEmitterCount();
            FParticleStatManager::GetInstance().AddTotalEmitterCount(1);

            // 통계 수집: 빔 파티클 카운트
            FParticleStatManager::GetInstance().AddBeamParticleCount(Source.ActiveParticleCount);

            // 빔 드로우 콜 통계 (빔은 보통 1개의 드로우콜)
            if (Source.ActiveParticleCount > 0)
            {
                FParticleStatManager::GetInstance().IncrementBeamDrawCallCount();
            }
        }
        else if (Source.eEmitterType == EDET_Ribbon)
        {
            // 통계 수집: 리본 에미터 카운트
            FParticleStatManager::GetInstance().IncrementRibbonEmitterCount();
            FParticleStatManager::GetInstance().AddTotalEmitterCount(1);

            // 통계 수집: 리본 파티클 카운트
            FParticleStatManager::GetInstance().AddRibbonParticleCount(Source.ActiveParticleCount);

            // 리본 드로우 콜 통계 (리본은 보통 1개의 드로우콜)
            if (Source.ActiveParticleCount > 0)
            {
                FParticleStatManager::GetInstance().IncrementRibbonDrawCallCount();
            }
        }
    }

    // 3. 스프라이트 에미터 렌더링
    if (!SpriteInstanceData.empty() && SpriteMaterial)
    {
        RenderSpriteParticles(SpriteInstanceData, SpriteMaterial, OutMeshBatchElements);
    }

    // 4. 메시 에미터 렌더링
    if (!MeshInstanceData.empty() && MeshMaterial && MeshToRender)
    {
        RenderMeshParticles(MeshInstanceData, MeshMaterial, MeshToRender, OutMeshBatchElements);
    }
}

// 스프라이트 파티클 렌더링 (GPU 인스턴싱)
void UParticleSystemComponent::RenderSpriteParticles(
    const TArray<FParticleInstanceData>& InstanceData,
    UMaterialInterface* Material,
    TArray<FMeshBatchElement>& OutMeshBatchElements)
{
    // 1. 공유 버퍼 매니저에 데이터 업로드
    FParticleInstanceBufferManager& BufferManager = FParticleInstanceBufferManager::Get();
    ID3D11ShaderResourceView* InstanceSRV = BufferManager.UpdateAndGetSRV(InstanceData);
    if (!InstanceSRV)
    {
        UE_LOG("[RenderSpriteParticles][Warning] Failed to update instance buffer.");
        return;
    }

    // 2. 렌더링 리소스 준비
    UShader* ShaderToUse = UResourceManager::GetInstance().Load<UShader>("Shaders/Materials/UberLit.hlsl");
    UQuad* ParticleQuad = UResourceManager::GetInstance().Get<UQuad>("BillboardQuad");
    if (!ShaderToUse || !ParticleQuad || ParticleQuad->GetIndexCount() == 0)
    {
        return;
    }

    // 3. 셰이더 매크로 설정 (PARTICLE_SPRITE 활성화)
    TArray<FShaderMacro> ShaderMacros;
    ShaderMacros.push_back({"PARTICLE_SPRITE", "1"});
    ShaderMacros.push_back({"LIGHTING_MODEL_PHONG", "1"});

    FShaderVariant* ShaderVariant = ShaderToUse->GetOrCompileShaderVariant(ShaderMacros);
    if (!ShaderVariant)
    {
        return;
    }

    // 4. 단일 FMeshBatchElement 생성 (인스턴싱)
    FMeshBatchElement BatchElement;

    BatchElement.VertexShader = ShaderVariant->VertexShader;
    BatchElement.PixelShader = ShaderVariant->PixelShader;
    BatchElement.InputLayout = ShaderVariant->InputLayout;
    BatchElement.Material = Material;
    BatchElement.VertexBuffer = ParticleQuad->GetVertexBuffer();
    BatchElement.IndexBuffer = ParticleQuad->GetIndexBuffer();
    BatchElement.VertexStride = ParticleQuad->GetVertexStride();

    BatchElement.IndexCount = ParticleQuad->GetIndexCount();
    BatchElement.StartIndex = 0;
    BatchElement.BaseVertexIndex = 0;
    BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    BatchElement.InstanceCount = static_cast<uint32>(InstanceData.size());
    BatchElement.ParticleInstanceSRV = InstanceSRV;

    BatchElement.WorldMatrix = FMatrix::Identity();
    BatchElement.InstanceColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Material의 텍스처 시스템을 사용 (InstanceShaderResourceView 설정하지 않음)
    // 렌더러가 Material에서 자동으로 텍스처를 조회합니다

    BatchElement.ObjectID = InternalIndex;

    OutMeshBatchElements.Add(BatchElement);

    // 스프라이트 드로우 콜 통계 수집 (GPU 인스턴싱으로 1번의 드로우콜)
    FParticleStatManager::GetInstance().IncrementSpriteDrawCallCount();
}

// 메시 파티클 렌더링 (GPU 인스턴싱)
void UParticleSystemComponent::RenderMeshParticles(
    const TArray<FParticleInstanceData>& InstanceData,
    UMaterialInterface* Material,
    UStaticMesh* Mesh,
    TArray<FMeshBatchElement>& OutMeshBatchElements)
{
    // 1. 공유 버퍼 매니저에 데이터 업로드
    FParticleInstanceBufferManager& BufferManager = FParticleInstanceBufferManager::Get();
    ID3D11ShaderResourceView* InstanceSRV = BufferManager.UpdateAndGetSRV(InstanceData);
    if (!InstanceSRV)
    {
        UE_LOG("[RenderMeshParticles][Warning] Failed to update instance buffer.");
        return;
    }

    // 2. 셰이더 로드 (Material 무시, 항상 UberLit.hlsl 사용)
    UShader* MeshShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Materials/UberLit.hlsl");
    if (!MeshShader)
    {
        UE_LOG("[RenderMeshParticles] ERROR: Failed to load UberLit.hlsl shader!");
        return;
    }

    // 3. 셰이더 매크로 (Particle Mesh + Phong Shading)
    TArray<FShaderMacro> MeshMacros;
    MeshMacros.push_back({"PARTICLE_MESH", "1"});
    MeshMacros.push_back({"LIGHTING_MODEL_PHONG", "1"});

    FShaderVariant* MeshVariant = MeshShader->GetOrCompileShaderVariant(MeshMacros);
    if (!MeshVariant)
    {
        return;
    }

    // 4. 단일 FMeshBatchElement 생성 (인스턴싱)
    FMeshBatchElement MeshBatch;

    MeshBatch.VertexShader = MeshVariant->VertexShader;
    MeshBatch.PixelShader = MeshVariant->PixelShader;
    MeshBatch.InputLayout = MeshVariant->InputLayout;
    MeshBatch.Material = Material;
    MeshBatch.VertexBuffer = Mesh->GetVertexBuffer();
    MeshBatch.IndexBuffer = Mesh->GetIndexBuffer();
    MeshBatch.VertexStride = Mesh->GetVertexStride();

    MeshBatch.IndexCount = Mesh->GetIndexCount();
    MeshBatch.StartIndex = 0;
    MeshBatch.BaseVertexIndex = 0;
    MeshBatch.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    MeshBatch.InstanceCount = static_cast<uint32>(InstanceData.size());
    MeshBatch.ParticleInstanceSRV = InstanceSRV;

    MeshBatch.WorldMatrix = FMatrix::Identity();
    MeshBatch.InstanceColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Material의 텍스처 시스템을 사용 (InstanceShaderResourceView 설정하지 않음)
    // 렌더러가 Material에서 자동으로 텍스처를 조회합니다

    MeshBatch.ObjectID = InternalIndex;

    OutMeshBatchElements.Add(MeshBatch);

    // 메시 드로우 콜 통계 수집 (GPU 인스턴싱으로 1번의 드로우콜)
    FParticleStatManager::GetInstance().IncrementMeshDrawCallCount();
}


// // 모든 파티클을 즉시 중지하고 메모리를 정리합니다. (강제 종료)
// void UParticleSystemComponent::KillParticlesAndCleanUp()
// {
//     ;
// }
//
// // 이 컴포넌트가 현재 재생 중인 이펙트를 멈추고 메모리를 해제합니다. (소멸자 등에서 호출)
// void UParticleSystemComponent::BeginDestroy()
// {
//     ;
// }