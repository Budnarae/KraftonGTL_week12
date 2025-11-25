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
#include "MeshBatchElement.h"
#include "SceneView.h"
#include "ResourceManager.h"
#include "Material.h"
#include "Shader.h"
#include "Quad.h"
#include "Texture.h"
#include "StaticMesh.h"
#include "Actor.h"

UParticleSystemComponent::UParticleSystemComponent()
{
    bCanEverTick = true;
}

UParticleSystemComponent::~UParticleSystemComponent()
{
    Deactivate();

    // 빔 동적 버퍼 해제
    if (BeamVertexBuffer)
    {
        BeamVertexBuffer->Release();
        BeamVertexBuffer = nullptr;
    }
    if (BeamIndexBuffer)
    {
        BeamIndexBuffer->Release();
        BeamIndexBuffer = nullptr;
    }

    // 리본 동적 버퍼 해제
    if (RibbonVertexBuffer)
    {
        RibbonVertexBuffer->Release();
        RibbonVertexBuffer = nullptr;
    }
    if (RibbonIndexBuffer)
    {
        RibbonIndexBuffer->Release();
        RibbonIndexBuffer = nullptr;
    }
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

    // D3D 버퍼는 복사되면 안 되므로 nullptr로 초기화 (복사본이 자체 버퍼 생성)
    BeamVertexBuffer = nullptr;
    BeamIndexBuffer = nullptr;
    BeamVertexBufferSize = 0;
    BeamIndexBufferSize = 0;

    RibbonVertexBuffer = nullptr;
    RibbonIndexBuffer = nullptr;
    RibbonVertexBufferSize = 0;
    RibbonIndexBufferSize = 0;

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
    
    CurrentLODLevel = InCurrentLODLevel;

    // 템플릿의 하위 에미터들에도 Current Level을 설정한다.
    if (!Template) return;

    for (UParticleEmitter* ParticleEmitter : Template->GetEmitters())
    {
        ParticleEmitter->SetCurrentLODLevel(CurrentLODLevel);
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

    // TODO: 현재는 최소 구현으로 무조건 LOD == 0이라고 설정한다. 추후 추가 구현 필요
    // SetCurrentLODLevel을 호출하여 모든 Emitter의 CurrentLODLevel도 함께 설정
    SetCurrentLODLevel(0);
    for (UParticleEmitter* Emitter : Template->GetEmitters())
    {
        // 페이로드 관련 정보 초기화
        Emitter->CacheEmitterModuleInfo();
        
        FParticleEmitterInstance* Instance = new FParticleEmitterInstance;

        // 템플릿 및 소유자 참조
        Instance->SpriteTemplate = Emitter;
        Instance->OwnerComponent = this;

        // LOD 및 모듈 참조
        Instance->CurrentLODLevelIndex = CurrentLODLevel;
        Instance->CurrentLODLevel = Emitter->GetCurrentLODLevelInstance();

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
        default:
            // 미구현 타입 (Beam, Ribbon 등)
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
    const static FVector Location = FVector(0, 0, 0);
    const static FVector Velocity = FVector(0, 0, 0.1f);

    // 이전 틱의 collision events 클리어
    ClearCollisionEvents();

    ElapsedTime += DeltaTime;

    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (Instance)
        {
            // 1. Update 함수를 먼저 호출하여 SpawnNum 계산 및 기존 파티클 업데이트
            Instance->Update(DeltaTime);

            // 2. 계산된 SpawnNum만큼 새 파티클 생성
            Instance->SpawnParticles(
                ElapsedTime,
                0.01f,
                Location,
                Velocity,
                nullptr
            );
        }
    }

    // 3. 시뮬레이션 완료 후 렌더링용 Dynamic Data 생성
    CreateDynamicData();
}

// 렌더링 관련 함수
void UParticleSystemComponent::CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View)
{
    // 1. 유효성 검사
    if (!Template || DynamicEmitterData.empty())
    {
        return;
    }

    // 2. 타입별로 데이터 수집 (Sprite / Mesh 분리)
    TArray<FParticleInstanceData> SpriteInstanceData;
    TArray<FParticleInstanceData> MeshInstanceData;
    UMaterial* SpriteMaterial = nullptr;
    UMaterial* MeshMaterial = nullptr;
    UStaticMesh* MeshToRender = nullptr;

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

            BatchElement.ObjectID = InternalIndex;
            OutMeshBatchElements.Add(BatchElement);

        // 타입별 분기
        if (Source.eEmitterType == EDET_Sprite)
        {
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
            for (int32 i = 0; i < Source.ActiveParticleCount; ++i)
            {
                int32 ParticleIndex = ParticleIndices ? ParticleIndices[i] : i;
                DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndex);

                FParticleInstanceData InstanceData;
                InstanceData.FillFromParticle(Particle, ComponentLocation, bEnableCameraFacing);
                SpriteInstanceData.Add(InstanceData);
            }
        }
        else if (Source.eEmitterType == EDET_Mesh)
        {
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
            for (int32 i = 0; i < Source.ActiveParticleCount; ++i)
            {
                int32 ParticleIndex = ParticleIndices ? ParticleIndices[i] : i;
                DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndex);

                FParticleInstanceData InstanceData;
                InstanceData.FillFromParticle(Particle, ComponentLocation, bEnableCameraFacing);
                MeshInstanceData.Add(InstanceData);
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
    UMaterial* Material,
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
}

// 메시 파티클 렌더링 (개별 드로우)
void UParticleSystemComponent::RenderMeshParticles(
    const TArray<FParticleInstanceData>& InstanceData,
    UMaterial* Material,
    UStaticMesh* Mesh,
    TArray<FMeshBatchElement>& OutMeshBatchElements)
{
    // 1. 셰이더 로드 (Material 무시, 항상 UberLit.hlsl 사용)
    UShader* MeshShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Materials/UberLit.hlsl");
    if (!MeshShader)
    {
        UE_LOG("[RenderMeshParticles] ERROR: Failed to load UberLit.hlsl shader!");
        return;
    }

    // 2. 셰이더 매크로 (Particle Mesh + Phong Shading)
    TArray<FShaderMacro> MeshMacros;
    MeshMacros.push_back({"PARTICLE_MESH", "1"});
    MeshMacros.push_back({"LIGHTING_MODEL_PHONG", "1"});

    FShaderVariant* MeshVariant = MeshShader->GetOrCompileShaderVariant(MeshMacros);
    if (!MeshVariant)
    {
        return;
    }

    // 3. 각 파티클을 개별 드로우콜로 렌더링
    for (const FParticleInstanceData& Data : InstanceData)
    {
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

        // 파티클 Transform을 WorldMatrix로 변환
        FMatrix ScaleMatrix = FMatrix::MakeScale(FVector(Data.Size.X, Data.Size.Y, Data.Size.X));
        FMatrix RotationMatrix = FQuat::FromAxisAngle(FVector(0.0f, 0.0f, 1.0f), Data.Rotation).ToMatrix();
        FMatrix TranslationMatrix = FMatrix::MakeTranslation(Data.Position);
        MeshBatch.WorldMatrix = ScaleMatrix * RotationMatrix * TranslationMatrix;

        MeshBatch.InstanceColor = Data.Color;
        MeshBatch.InstanceCount = 1;
        MeshBatch.ParticleInstanceSRV = nullptr;

        // Material의 텍스처 시스템을 사용 (InstanceShaderResourceView 설정하지 않음)
        // 렌더러가 Material에서 자동으로 텍스처를 조회합니다

        MeshBatch.ObjectID = InternalIndex;

        OutMeshBatchElements.Add(MeshBatch);
    }
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