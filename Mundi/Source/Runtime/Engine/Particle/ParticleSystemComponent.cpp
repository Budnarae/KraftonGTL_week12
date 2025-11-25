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

        FDynamicSpriteEmitterData* DynamicData = new FDynamicSpriteEmitterData();
        DynamicData->Init(Instance, i);
        DynamicEmitterData.Add(DynamicData);
    }
}

// Dynamic Data 해제
void UParticleSystemComponent::ReleaseDynamicData()
{
    for (FDynamicSpriteEmitterData* DynamicData : DynamicEmitterData)
    {
        if (DynamicData)
        {
            delete DynamicData;
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

    // 2. 각 에미터 인스턴스를 순회
    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (!Instance)
        {
            continue;
        }

        // 3. 렌더링에 필요한 리소스 준비
        UParticleModuleRequired* RequiredModule = Instance->SpriteTemplate->GetCurrentLODLevelInstance()->GetRequiredModule();
        if (!RequiredModule)
        {
            continue;
        }

        UMaterialInterface* ParticleMaterial = RequiredModule->GetMaterial();
        if (!ParticleMaterial)
        {
            UE_LOG("[UParticleSystemComponent::CollectMeshBatches][Warning] No material found.");
            continue;
        }

        UShader* ShaderToUse = ParticleMaterial->GetShader();
        if (!ShaderToUse)
        {
            UE_LOG("[UParticleSystemComponent::CollectMeshBatches][Warning] No shader found.");
            continue;
        }

        // 4. 공유 리소스 준비 (Billboard Quad)
        UQuad* ParticleQuad = UResourceManager::GetInstance().Get<UQuad>("BillboardQuad");
        if (!ParticleQuad || ParticleQuad->GetIndexCount() == 0)
        {
            UE_LOG("[UParticleSystemComponent::CollectMeshBatches][Warning] Billboard quad not found.");
            continue;
        }

        // 5. 셰이더 컴파일
        FShaderVariant* ShaderVariant = ShaderToUse->GetOrCompileShaderVariant(ParticleMaterial->GetShaderMacros());
        if (!ShaderVariant)
        {
            UE_LOG("[UParticleSystemComponent::CollectMeshBatches][Warning] Shader compilation failed.");
            continue;
        }

        // 5.5. TypeDataModule 체크 - Beam 타입인지 확인
        UParticleLODLevel* LODLevel = Instance->SpriteTemplate->GetCurrentLODLevelInstance();
        UParticleModuleTypeDataBase* TypeDataModule = LODLevel ? LODLevel->GetTypeDataModule() : nullptr;

        // Beam 타입이면 빔 렌더링 (Triangle Strip 방식 - 정점 공유로 틈 없음)
        if (TypeDataModule && TypeDataModule->GetEmitterType() == EDynamicEmitterType::EDET_Beam)
        {
            UParticleModuleTypeDataBeam* BeamModule = static_cast<UParticleModuleTypeDataBeam*>(TypeDataModule);

            // 빔 타겟/소스 위치 동적 업데이트
            if (BeamTargetActor && BeamModule->GetBeamMethod() == EBeamMethod::Target)
            {
                BeamModule->SetTargetPoint(BeamTargetActor->GetActorLocation());
            }
            if (BeamSourceActor)
            {
                // SourcePoint는 EmitterLocation 기준 로컬 좌표
                BeamModule->SetSourcePoint(BeamSourceActor->GetActorLocation() - GetWorldLocation());
            }

            // 빔 포인트 계산 (시간을 전달하여 동적 노이즈)
            TArray<FVector> BeamPoints;
            TArray<float> BeamWidths;
            BeamModule->CalculateBeamPoints(GetWorldLocation(), BeamPoints, BeamWidths, ElapsedTime);

            if (BeamPoints.Num() < 2)
                continue;

            // 1. 각 포인트에 대해 Right 벡터 계산
            TArray<FVector> PointRightVectors;
            PointRightVectors.SetNum(BeamPoints.Num());

            for (int32 i = 0; i < BeamPoints.Num(); ++i)
            {
                // 이 포인트에서의 Forward 방향 계산 (인접 세그먼트들의 평균)
                FVector Forward = FVector::Zero();

                if (i > 0)
                    Forward += (BeamPoints[i] - BeamPoints[i - 1]).GetNormalized();
                if (i < BeamPoints.Num() - 1)
                    Forward += (BeamPoints[i + 1] - BeamPoints[i]).GetNormalized();

                if (Forward.Size() < 0.001f)
                    Forward = FVector(1, 0, 0);
                else
                    Forward = Forward.GetNormalized();

                // 카메라 방향
                FVector ToCamera = (View->ViewLocation - BeamPoints[i]).GetNormalized();

                // Right 벡터 계산
                FVector Right = FVector::Cross(Forward, ToCamera);

                // 폴백 처리
                if (Right.Size() < 0.001f)
                {
                    Right = FVector::Cross(Forward, FVector(0, 0, 1));
                    if (Right.Size() < 0.001f)
                        Right = FVector::Cross(Forward, FVector(0, 1, 0));
                }

                PointRightVectors[i] = Right.GetNormalized();
            }

            // 2. 정점 데이터 생성 (각 포인트당 2개 정점: top, bottom)
            uint32 NumPoints = static_cast<uint32>(BeamPoints.Num());
            uint32 NumVertices = NumPoints * 2;
            uint32 NumSegments = NumPoints - 1;
            uint32 NumIndices = NumSegments * 6;  // 세그먼트당 2개 삼각형 = 6개 인덱스

            TArray<FBillboardVertex> Vertices;
            TArray<uint32> Indices;
            Vertices.SetNum(NumVertices);
            Indices.SetNum(NumIndices);

            // 정점 생성
            for (uint32 i = 0; i < NumPoints; ++i)
            {
                FVector Point = BeamPoints[i];
                FVector Right = PointRightVectors[i];
                float Width = BeamWidths[i];
                float U = static_cast<float>(i) / static_cast<float>(NumPoints - 1);

                // Bottom 정점 (V = 0)
                Vertices[i * 2].WorldPosition = Point - Right * Width * 0.5f;
                Vertices[i * 2].UV = FVector2D(U, 0.0f);

                // Top 정점 (V = 1)
                Vertices[i * 2 + 1].WorldPosition = Point + Right * Width * 0.5f;
                Vertices[i * 2 + 1].UV = FVector2D(U, 1.0f);
            }

            // 인덱스 생성 (Triangle List)
            for (uint32 i = 0; i < NumSegments; ++i)
            {
                uint32 BaseIndex = i * 6;
                uint32 V0 = i * 2;      // 현재 bottom
                uint32 V1 = i * 2 + 1;  // 현재 top
                uint32 V2 = i * 2 + 2;  // 다음 bottom
                uint32 V3 = i * 2 + 3;  // 다음 top

                // 삼각형 1: V0 - V1 - V2
                Indices[BaseIndex + 0] = V0;
                Indices[BaseIndex + 1] = V1;
                Indices[BaseIndex + 2] = V2;

                // 삼각형 2: V1 - V3 - V2
                Indices[BaseIndex + 3] = V1;
                Indices[BaseIndex + 4] = V3;
                Indices[BaseIndex + 5] = V2;
            }

            // 3. 동적 버퍼 생성/업데이트
            D3D11RHI* RHIDevice = GEngine.GetRHIDevice();
            ID3D11Device* Device = RHIDevice->GetDevice();

            // 버텍스 버퍼
            if (!BeamVertexBuffer || BeamVertexBufferSize < NumVertices)
            {
                if (BeamVertexBuffer)
                    BeamVertexBuffer->Release();

                D3D11_BUFFER_DESC VBDesc = {};
                VBDesc.Usage = D3D11_USAGE_DYNAMIC;
                VBDesc.ByteWidth = NumVertices * sizeof(FBillboardVertex);
                VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                VBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                Device->CreateBuffer(&VBDesc, nullptr, &BeamVertexBuffer);
                BeamVertexBufferSize = NumVertices;
            }

            // 인덱스 버퍼
            if (!BeamIndexBuffer || BeamIndexBufferSize < NumIndices)
            {
                if (BeamIndexBuffer)
                    BeamIndexBuffer->Release();

                D3D11_BUFFER_DESC IBDesc = {};
                IBDesc.Usage = D3D11_USAGE_DYNAMIC;
                IBDesc.ByteWidth = NumIndices * sizeof(uint32);
                IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
                IBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                Device->CreateBuffer(&IBDesc, nullptr, &BeamIndexBuffer);
                BeamIndexBufferSize = NumIndices;
            }

            // 버퍼 업데이트
            ID3D11DeviceContext* Context = RHIDevice->GetDeviceContext();

            D3D11_MAPPED_SUBRESOURCE MappedVB;
            Context->Map(BeamVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedVB);
            memcpy(MappedVB.pData, Vertices.GetData(), NumVertices * sizeof(FBillboardVertex));
            Context->Unmap(BeamVertexBuffer, 0);

            D3D11_MAPPED_SUBRESOURCE MappedIB;
            Context->Map(BeamIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedIB);
            memcpy(MappedIB.pData, Indices.GetData(), NumIndices * sizeof(uint32));
            Context->Unmap(BeamIndexBuffer, 0);

            // 4. BatchElement 생성 (단일 DrawCall)
            FMeshBatchElement BatchElement;
            BatchElement.VertexShader = ShaderVariant->VertexShader;
            BatchElement.PixelShader = ShaderVariant->PixelShader;
            BatchElement.InputLayout = ShaderVariant->InputLayout;
            BatchElement.Material = ParticleMaterial;
            BatchElement.VertexBuffer = BeamVertexBuffer;
            BatchElement.IndexBuffer = BeamIndexBuffer;
            BatchElement.VertexStride = sizeof(FBillboardVertex);
            BatchElement.IndexCount = NumIndices;
            BatchElement.StartIndex = 0;
            BatchElement.BaseVertexIndex = 0;
            BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            // WorldMatrix는 Identity (정점이 이미 월드 좌표)
            BatchElement.WorldMatrix = FMatrix::Identity();

            // 빔 색상 (GlowIntensity 적용)
            FVector4 BeamColor = BeamModule->GetBeamColor();
            float GlowIntensity = BeamModule->GetGlowIntensity();
            BatchElement.InstanceColor = FLinearColor(
                BeamColor.X * GlowIntensity,
                BeamColor.Y * GlowIntensity,
                BeamColor.Z * GlowIntensity,
                BeamColor.W
            );

            // UV 매핑 (전체 빔에서 0~1, 셰이더에서 리매핑 불필요)
            BatchElement.UVStart = 0.0f;
            BatchElement.UVEnd = 1.0f;

            // 텍스처 사용 여부
            bool bUseTexture = BeamModule->GetUseTexture();
            BatchElement.UseTexture = bUseTexture ? 1.0f : 0.0f;

            // 빔은 CLAMP 샘플러 사용 (텍스처 경계 아티팩트 방지)
            BatchElement.SamplerType = 1;

            // 텍스처 설정
            if (bUseTexture && ParticleMaterial)
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

            BatchElement.ObjectID = InternalIndex;
            OutMeshBatchElements.Add(BatchElement);

            continue; // 빔 렌더링 완료, 스프라이트 렌더링 스킵
        }

        // Ribbon 타입이면 리본 렌더링
        if (TypeDataModule && TypeDataModule->GetEmitterType() == EDynamicEmitterType::EDET_Ribbon)
        {
            UParticleModuleTypeDataRibbon* RibbonModule = static_cast<UParticleModuleTypeDataRibbon*>(TypeDataModule);

            // 리본 포인트 계산 (파티클들로부터)
            TArray<FVector> RibbonPoints;
            TArray<float> RibbonWidths;
            TArray<FLinearColor> RibbonColors;
            RibbonModule->BuildRibbonFromParticles(Instance, GetWorldLocation(), RibbonPoints, RibbonWidths, RibbonColors);

            if (RibbonPoints.Num() < 2)
                continue;

            // 1. 각 포인트에 대해 Right 벡터 계산 (카메라를 향하도록)
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

                // 카메라를 향하는 Right 벡터
                FVector ToCamera = (View->ViewLocation - RibbonPoints[i]).GetNormalized();
                FVector Right = FVector::Cross(Forward, ToCamera);

                // 폴백 처리
                if (Right.Size() < 0.001f)
                {
                    Right = FVector::Cross(Forward, FVector(0, 0, 1));
                    if (Right.Size() < 0.001f)
                        Right = FVector::Cross(Forward, FVector(0, 1, 0));
                }

                PointRightVectors[i] = Right.GetNormalized();
            }

            // 2. 정점 데이터 생성
            uint32 NumPoints = static_cast<uint32>(RibbonPoints.Num());
            uint32 NumVertices = NumPoints * 2;
            uint32 NumSegments = NumPoints - 1;
            uint32 NumIndices = NumSegments * 6;

            TArray<FBillboardVertex> Vertices;
            TArray<uint32> Indices;
            Vertices.SetNum(NumVertices);
            Indices.SetNum(NumIndices);

            // 텍스처 반복 계수
            float TextureRepeat = RibbonModule->GetTextureRepeat();

            // 정점 생성
            for (uint32 i = 0; i < NumPoints; ++i)
            {
                FVector Point = RibbonPoints[i];
                FVector Right = PointRightVectors[i];
                float Width = RibbonWidths[i];
                float U = static_cast<float>(i) / static_cast<float>(NumPoints - 1) * TextureRepeat;
                FLinearColor VertexColor = RibbonColors[i];  // 각 세그먼트의 색상

                // Bottom 정점 (V = 0)
                Vertices[i * 2].WorldPosition = Point - Right * Width * 0.5f;
                Vertices[i * 2].UV = FVector2D(U, 0.0f);
                Vertices[i * 2].Color = VertexColor;

                // Top 정점 (V = 1)
                Vertices[i * 2 + 1].WorldPosition = Point + Right * Width * 0.5f;
                Vertices[i * 2 + 1].UV = FVector2D(U, 1.0f);
                Vertices[i * 2 + 1].Color = VertexColor;
            }

            // 인덱스 생성 (Triangle List)
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

            // 3. 동적 버퍼 생성/업데이트
            D3D11RHI* RHIDevice = GEngine.GetRHIDevice();
            ID3D11Device* Device = RHIDevice->GetDevice();

            // 버텍스 버퍼
            if (!RibbonVertexBuffer || RibbonVertexBufferSize < NumVertices)
            {
                if (RibbonVertexBuffer)
                    RibbonVertexBuffer->Release();

                D3D11_BUFFER_DESC VBDesc = {};
                VBDesc.Usage = D3D11_USAGE_DYNAMIC;
                VBDesc.ByteWidth = NumVertices * sizeof(FBillboardVertex);
                VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                VBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                Device->CreateBuffer(&VBDesc, nullptr, &RibbonVertexBuffer);
                RibbonVertexBufferSize = NumVertices;
            }

            // 인덱스 버퍼
            if (!RibbonIndexBuffer || RibbonIndexBufferSize < NumIndices)
            {
                if (RibbonIndexBuffer)
                    RibbonIndexBuffer->Release();

                D3D11_BUFFER_DESC IBDesc = {};
                IBDesc.Usage = D3D11_USAGE_DYNAMIC;
                IBDesc.ByteWidth = NumIndices * sizeof(uint32);
                IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
                IBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                Device->CreateBuffer(&IBDesc, nullptr, &RibbonIndexBuffer);
                RibbonIndexBufferSize = NumIndices;
            }

            // 버퍼 업데이트
            ID3D11DeviceContext* Context = RHIDevice->GetDeviceContext();

            D3D11_MAPPED_SUBRESOURCE MappedVB;
            Context->Map(RibbonVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedVB);
            memcpy(MappedVB.pData, Vertices.GetData(), NumVertices * sizeof(FBillboardVertex));
            Context->Unmap(RibbonVertexBuffer, 0);

            D3D11_MAPPED_SUBRESOURCE MappedIB;
            Context->Map(RibbonIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedIB);
            memcpy(MappedIB.pData, Indices.GetData(), NumIndices * sizeof(uint32));
            Context->Unmap(RibbonIndexBuffer, 0);

            // 4. BatchElement 생성
            FMeshBatchElement BatchElement;
            BatchElement.VertexShader = ShaderVariant->VertexShader;
            BatchElement.PixelShader = ShaderVariant->PixelShader;
            BatchElement.InputLayout = ShaderVariant->InputLayout;
            BatchElement.Material = ParticleMaterial;
            BatchElement.VertexBuffer = RibbonVertexBuffer;
            BatchElement.IndexBuffer = RibbonIndexBuffer;
            BatchElement.VertexStride = sizeof(FBillboardVertex);
            BatchElement.IndexCount = NumIndices;
            BatchElement.StartIndex = 0;
            BatchElement.BaseVertexIndex = 0;
            BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            BatchElement.WorldMatrix = FMatrix::Identity();

            // 리본 색상 (정점 색상을 사용하므로 InstanceColor는 흰색으로 설정)
            // 정점 색상 * InstanceColor가 최종 색상이 됨
            BatchElement.InstanceColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

            BatchElement.UVStart = 0.0f;
            BatchElement.UVEnd = 1.0f;

            // 텍스처 설정
            bool bUseTexture = RibbonModule->GetUseTexture();
            BatchElement.UseTexture = bUseTexture ? 1.0f : 0.0f;

            // 리본은 CLAMP 샘플러 사용 (텍스처 경계 아티팩트 방지)
            BatchElement.SamplerType = 1;

            if (bUseTexture && ParticleMaterial)
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

            BatchElement.ObjectID = InternalIndex;
            OutMeshBatchElements.Add(BatchElement);

            continue; // 리본 렌더링 완료, 스프라이트 렌더링 스킵
        }

        // 6. 각 파티클에 대해 FMeshBatchElement 생성 (스프라이트)
        // 스프라이트는 ActiveParticles가 0이면 스킵
        if (Instance->ActiveParticles == 0)
        {
            continue;
        }

        // TODO: 현재는 각 파티클마다 별도의 Draw Call을 생성 (최소 구현)
        // 추후 인스턴싱이나 동적 버퍼로 최적화 필요
        for (int32 ParticleIndex = 0; ParticleIndex < Instance->ActiveParticles; ParticleIndex++)
        {
            DECLARE_PARTICLE_PTR(Particle, Instance->ParticleData + Instance->ParticleStride * ParticleIndex);

            // 7. FMeshBatchElement 생성
            FMeshBatchElement BatchElement;

            // --- 정렬 키 ---
            BatchElement.VertexShader = ShaderVariant->VertexShader;
            BatchElement.PixelShader = ShaderVariant->PixelShader;
            BatchElement.InputLayout = ShaderVariant->InputLayout;
            BatchElement.Material = ParticleMaterial;
            BatchElement.VertexBuffer = ParticleQuad->GetVertexBuffer();
            BatchElement.IndexBuffer = ParticleQuad->GetIndexBuffer();
            BatchElement.VertexStride = ParticleQuad->GetVertexStride();

            // --- 드로우 데이터 ---
            BatchElement.IndexCount = ParticleQuad->GetIndexCount();
            BatchElement.StartIndex = 0;
            BatchElement.BaseVertexIndex = 0;
            BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            // --- 인스턴스 데이터 ---
            // 파티클 위치 = 로컬 파티클 위치 + 컴포넌트 월드 위치
            FVector WorldParticleLocation = Particle->Location + GetWorldLocation();
            FMatrix ScaleMatrix = FMatrix::MakeScale(Particle->Size);
            FMatrix TranslationMatrix = FMatrix::MakeTranslation(WorldParticleLocation);
            BatchElement.WorldMatrix = ScaleMatrix * TranslationMatrix;

            // 파티클 색상
            BatchElement.InstanceColor = Particle->Color;

            // Material의 텍스처 가져오기 (Diffuse 텍스처)
            UTexture* DiffuseTexture = nullptr;
            if (ParticleMaterial->HasTexture(EMaterialTextureSlot::Diffuse))
            {
                DiffuseTexture = ParticleMaterial->GetTexture(EMaterialTextureSlot::Diffuse);
            }

            // 텍스처가 없으면 기본 텍스처 사용 (테스트용)
            if (!DiffuseTexture)
            {
                DiffuseTexture = UResourceManager::GetInstance().Load<UTexture>(GDataDir + "/Textures/grass.jpg");
            }

            if (DiffuseTexture && DiffuseTexture->GetShaderResourceView())
            {
                BatchElement.InstanceShaderResourceView = DiffuseTexture->GetShaderResourceView();
            }
            else
            {
                BatchElement.InstanceShaderResourceView = nullptr;
            }

            // 오브젝트 ID (선택 시스템용)
            BatchElement.ObjectID = InternalIndex;

            OutMeshBatchElements.Add(BatchElement);
        }
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