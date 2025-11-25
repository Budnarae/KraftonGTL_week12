#include "pch.h"
#include "ParticleSystemActor.h"
#include "../Particle/ParticleSystemComponent.h"
#include "../Particle/ParticleSystem.h"
#include "../Particle/ParticleEmitter.h"
#include "../Particle/ParticleLODLevel.h"
#include "../Particle/ParticleModuleRequired.h"
#include "../Particle/ParticleModuleLocation.h"
#include "../Particle/ParticleModuleVelocity.h"
#include "../Particle/ParticleModuleSpawn.h"
#include "../Particle/ParticleModuleLifetime.h"
#include "../Particle/ParticleModuleColor.h"
#include "../Particle/ParticleModuleSize.h"
#include "../Particle/ParticleEmitterInstance.h"
#include "../Particle/ParticleData.h"
#include "Material.h"
#include "ResourceManager.h"
#include "StaticMesh.h"

AParticleSystemActor::AParticleSystemActor()
{
	ObjectName = "Particle System Actor";
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");

	// 루트 교체
	RootComponent = ParticleSystemComponent;

	// ============================================================
	// TEST CODE - MESH EMITTER
	// 메시 에미터 기능 테스트
	// ============================================================
	
	{
		// 1. ParticleSystem 템플릿 생성
		UParticleSystem* TestTemplate = NewObject<UParticleSystem>();
		TestTemplate->ObjectName = "TestMeshParticleSystem";

		// 2. Emitter 생성
		UParticleEmitter* TestEmitter = NewObject<UParticleEmitter>();
		TestEmitter->ObjectName = "TestMeshEmitter";
		TestEmitter->SetMaxParticleCount(30);

		// 3. LOD Level 0에 접근 (생성자에서 이미 RequiredModule이 생성됨)
		UParticleLODLevel* TestLODLevel = TestEmitter->GetParticleLODLevelWithIndex(0);
		if (TestLODLevel)
		{
			// 4. Required Module 설정
			UParticleModuleRequired* RequiredModule = TestLODLevel->GetRequiredModule();
			if (RequiredModule)
			{
				// 렌더링할 메시 로드 및 설정 (apple_mid 사용)
				UStaticMesh* MeshToUse = UResourceManager::GetInstance().Load<UStaticMesh>(GResourceDir + "/bitten_apple_mid.umesh");
				if (MeshToUse)
				{
					RequiredModule->SetMesh(MeshToUse);
					UE_LOG("[ParticleSystemActor] Mesh loaded and set to RequiredModule: %s", MeshToUse->GetAssetPathFileName().c_str());
					UE_LOG("[ParticleSystemActor] Mesh buffers - VB: %p, IB: %p, IndexCount: %d",
						MeshToUse->GetVertexBuffer(), MeshToUse->GetIndexBuffer(), MeshToUse->GetIndexCount());

					// 메시의 GroupInfo에서 Material 정보 가져오기
					if (MeshToUse->HasMaterial())
					{
						const TArray<FGroupInfo>& GroupInfos = MeshToUse->GetMeshGroupInfo();
						if (!GroupInfos.empty() && !GroupInfos[0].InitialMaterialName.empty())
						{
							UMaterial* ParticleMaterial = UResourceManager::GetInstance().Load<UMaterial>(GroupInfos[0].InitialMaterialName);
							if (ParticleMaterial)
							{
								RequiredModule->SetMaterial(ParticleMaterial);
								UE_LOG("[ParticleSystemActor] Material loaded from mesh GroupInfo: %s", GroupInfos[0].InitialMaterialName.c_str());
							}
						}
					}
				}
				else
				{
					UE_LOG("[ParticleSystemActor] ERROR: Failed to load apple_mid for mesh emitter!");
				}

				RequiredModule->SetSpawnRate(5.0f);   // 초당 5개 (메시는 무거우니 적게)
				RequiredModule->SetLifeTime(5.0f);    // 5초 수명
				RequiredModule->SetEmitterDuration(0.0f);
				RequiredModule->SetEmitterDelay(0.0f);
				RequiredModule->SetEnableCameraFacing(false);  // 메시는 빌보드 비활성화
			}

			// 5. Location Module
			UParticleModuleLocation* LocationModule = NewObject<UParticleModuleLocation>();
			if (LocationModule)
			{
				LocationModule->SetDistributionBox(FVector::Zero(), FVector(2.0f, 2.0f, 2.0f));
				TestLODLevel->AddSpawnModule(LocationModule);
			}

			// 6. Velocity Module
			UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
			if (VelocityModule)
			{
				VelocityModule->SetVelocityRange(FVector(-1.0f, -1.0f, 2.0f), FVector(1.0f, 1.0f, 4.0f));
				VelocityModule->SetStartVelocityRadialMin(0.5f);
				VelocityModule->SetStartVelocityRadialMax(1.5f);
				TestLODLevel->AddSpawnModule(VelocityModule);
			}

			// 7. Spawn Module
			UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
			if (SpawnModule)
			{
				SpawnModule->SetRateMin(3.0f);
				SpawnModule->SetRateMax(7.0f);
				SpawnModule->AddBurst(5, 0.0f);
				TestLODLevel->AddSpawnModule(SpawnModule);
			}

			// 8. Lifetime Module
			UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
			if (LifetimeModule)
			{
				LifetimeModule->SetLifetimeRange(4.0f, 6.0f);
				TestLODLevel->AddSpawnModule(LifetimeModule);
			}

			// 9. Color Module (랜덤 투명도 적용)
			UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
			if (ColorModule)
			{
				ColorModule->SetColorRange(FVector(0.5f, 0.8f, 1.0f), FVector(0.8f, 1.0f, 1.0f));
				ColorModule->SetAlphaRange(0.1f, 0.9f);  // 랜덤 투명도 (0.1 ~ 0.9)
				TestLODLevel->AddSpawnModule(ColorModule);
			}

			// 10. Size Module
			UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
			if (SizeModule)
			{
				SizeModule->SetUniformSize(0.3f, 0.7f);
				TestLODLevel->AddSpawnModule(SizeModule);
			}
		}

		// Emitter를 ParticleSystem에 추가
		TestTemplate->AddEmitter(TestEmitter);

		// ParticleSystemComponent에 템플릿 설정
		ParticleSystemComponent->SetTemplate(TestTemplate);

		// 파티클 시스템 활성화
		ParticleSystemComponent->Activate(false);

		// Activate 직후 EmitterInstance를 Mesh Emitter로 설정
		TArray<FParticleEmitterInstance*>& Instances = ParticleSystemComponent->GetSystemInstance();
		if (!Instances.empty() && Instances[0])
		{
			FParticleEmitterInstance* Instance = Instances[0];

			// 에미터 타입을 Mesh로 변경
			Instance->EmitterType = EDET_Mesh;
		}
	}
	
	// ============================================================
	// END OF MESH EMITTER TEST CODE
	// ============================================================

	// ============================================================
	// SPRITE EMITTER TEST CODE
	// 스프라이트 에미터를 테스트하려면 위의 메시 에미터 코드를 주석 처리하고
	// 아래 코드의 주석을 해제하세요.
	// ============================================================
	/*
	{
		// 1. ParticleSystem 템플릿 생성
		UParticleSystem* TestTemplate = NewObject<UParticleSystem>();
		TestTemplate->ObjectName = "TestParticleSystem";

		// 2. Emitter 생성 (생성자에서 자동으로 LODLevel 배열이 생성됨)
		UParticleEmitter* TestEmitter = NewObject<UParticleEmitter>();
		TestEmitter->ObjectName = "TestEmitter";
		TestEmitter->SetMaxParticleCount(100);

		// 3. LOD Level 0에 접근 (이미 생성되어 있음)
		UParticleLODLevel* TestLODLevel = TestEmitter->GetParticleLODLevelWithIndex(0);
		if (TestLODLevel)
		{
			TestLODLevel->CreateRequiredModule();

			// 4. Required Module 설정 (필수 모듈)
			UParticleModuleRequired* RequiredModule = TestLODLevel->GetRequiredModule();
			if (RequiredModule)
			{
				// 머티리얼 설정 (Billboard 셰이더 사용)
				UMaterial* ParticleMaterial = UResourceManager::GetInstance().Load<UMaterial>("MatID_1");
				if (ParticleMaterial)
				{
					RequiredModule->SetMaterial(ParticleMaterial);
				}

				// 파티클 생성 설정 - 가시성 좋게 높은 스폰율
				RequiredModule->SetSpawnRate(50.0f);  // 초당 50개 생성
				RequiredModule->SetLifeTime(3.0f); // 3초 수명
				RequiredModule->SetEmitterDuration(0.0f); // 0 = 무한 루프
				RequiredModule->SetEmitterDelay(0.0f);
			}

			// 5. Location Module 설정 (스폰 위치 분산)
			UParticleModuleLocation* LocationModule = NewObject<UParticleModuleLocation>();
			if (LocationModule)
			{
				// 박스 형태로 스폰 영역 설정 (중심: 0, 반경: 50)
				LocationModule->SetDistributionBox(FVector::Zero(), FVector(2.0f, 2.0f, 2.0f));
				TestLODLevel->AddSpawnModule(LocationModule);
			}

			// 6. Velocity Module 설정 (초기 속도)
			UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
			if (VelocityModule)
			{
				// 위쪽으로 솟아오르는 속도 설정 (Z축: 1~3 범위)
				VelocityModule->SetVelocityRange(
					FVector(-0.5f, -0.5f, 1.0f),  // Min: 약간의 수평 분산 + 위로
					FVector(0.5f, 0.5f, 3.0f)     // Max: 약간의 수평 분산 + 위로
				);

				// 방사형 속도 추가 (바깥으로 퍼지는 효과)
				VelocityModule->SetStartVelocityRadialMin(0.5f);
				VelocityModule->SetStartVelocityRadialMax(1.5f);

				TestLODLevel->AddSpawnModule(VelocityModule);
			}

			// 7. Spawn Module 설정 (스폰율 및 버스트)
			UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
			if (SpawnModule)
			{
				// 기본 스폰율: 초당 30~50개
				SpawnModule->SetRateMin(30.0f);
				SpawnModule->SetRateMax(50.0f);

				// 시작 시 버스트: 0초에 20개 한번에 생성
				SpawnModule->AddBurst(20, 0.0f);

				// 1초에 추가 버스트: 10~30개 랜덤
				SpawnModule->AddBurst(10, 30, 1.0f);

				TestLODLevel->AddSpawnModule(SpawnModule);
			}

			// 8. Lifetime Module 설정 (파티클 수명)
			UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
			if (LifetimeModule)
			{
				// 파티클 수명: 2~4초 랜덤
				LifetimeModule->SetLifetimeRange(2.0f, 4.0f);

				TestLODLevel->AddSpawnModule(LifetimeModule);
			}

			// 9. Color Module 설정 (초기 색상, 랜덤 투명도)
			UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
			if (ColorModule)
			{
				// 주황~노랑 색상 범위 (불꽃 느낌)
				ColorModule->SetColorRange(
					FVector(1.0f, 0.3f, 0.0f),   // Min: 주황
					FVector(1.0f, 0.8f, 0.2f)    // Max: 노랑
				);

				// 알파: 0.4~0.9 (랜덤 투명도)
				ColorModule->SetAlphaRange(0.4f, 0.9f);

				TestLODLevel->AddSpawnModule(ColorModule);
			}

			// 10. Size Module 설정 (초기 크기)
			UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
			if (SizeModule)
			{
				// 파티클 크기: 0.1~0.3 유닛 (작은 파티클)
				SizeModule->SetUniformSize(0.1f, 0.3f);

				TestLODLevel->AddSpawnModule(SizeModule);
			}
		}

		// Emitter를 ParticleSystem에 추가
		TestTemplate->AddEmitter(TestEmitter);

		// ParticleSystemComponent에 템플릿 설정
		ParticleSystemComponent->SetTemplate(TestTemplate);

		// 파티클 시스템 활성화
		ParticleSystemComponent->Activate(false);
	}
	*/
	// ============================================================
	// END OF SPRITE EMITTER TEST CODE
	// ============================================================
}

void AParticleSystemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

AParticleSystemActor::~AParticleSystemActor()
{

}

void AParticleSystemActor::SetParticleSystemComponent(UParticleSystemComponent* InParticleSystemComponent)
{
	ParticleSystemComponent = InParticleSystemComponent;
}

void AParticleSystemActor::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
	for (UActorComponent* Component : OwnedComponents)
	{
		if (UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(Component))
		{
			ParticleSystemComponent = PSC;
			break;
		}
	}
}

void AParticleSystemActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		ParticleSystemComponent = Cast<UParticleSystemComponent>(RootComponent);
	}
}
