#include "pch.h"
#include "ParticleSystemComponent.h"

#include "Keyboard.h"
#include "ParticleData.h"
#include "ParticleHelper.h"
#include "MeshBatchElement.h"
#include "SceneView.h"
#include "ResourceManager.h"
#include "Material.h"
#include "Shader.h"
#include "Quad.h"
#include "Texture.h"

UParticleSystemComponent::UParticleSystemComponent()
{
    bCanEverTick = true;
}

UParticleSystemComponent::~UParticleSystemComponent()
{
    Deactivate();
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
        if (!Instance || Instance->ActiveParticles == 0)
        {
            continue;
        }

        // 3. 렌더링에 필요한 리소스 준비
        UParticleModuleRequired* RequiredModule = Instance->SpriteTemplate->GetCurrentLODLevelInstance()->GetRequiredModule();
        if (!RequiredModule)
        {
            continue;
        }

        UMaterial* ParticleMaterial = RequiredModule->GetMaterial();
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

        // 6. 각 파티클에 대해 FMeshBatchElement 생성
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