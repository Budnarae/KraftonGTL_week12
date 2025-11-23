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
#include "Material.h"
#include "ResourceManager.h"

AParticleSystemActor::AParticleSystemActor()
{
	ObjectName = "Particle System Actor";
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");

	// 루트 교체
	RootComponent = ParticleSystemComponent;

	// ============================================================
	// TEST CODE - REMOVE LATER
	// 가시성 테스트를 위한 하드코딩된 파티클 시스템 설정
	// ============================================================
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
				UMaterial* ParticleMaterial = UResourceManager::GetInstance().Load<UMaterial>("Shaders/UI/Billboard.hlsl");
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
		}

		// Emitter를 ParticleSystem에 추가
		TestTemplate->AddEmitter(TestEmitter);

		// ParticleSystemComponent에 템플릿 설정
		ParticleSystemComponent->SetTemplate(TestTemplate);

		// 파티클 시스템 활성화
		ParticleSystemComponent->Activate(false);
	}
	// ============================================================
	// END OF TEST CODE
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
