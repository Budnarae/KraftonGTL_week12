#include "pch.h"
#include "ParticleSystemActor.h"
#include "../Particle/ParticleSystemComponent.h"
#include "../Particle/ParticleSystem.h"
#include "../Particle/ParticleEmitter.h"
#include "../Particle/ParticleLODLevel.h"
#include "../Particle/ParticleModuleRequired.h"
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
				// 머티리얼 설정 (기본 파티클 머티리얼 사용)
				UMaterial* ParticleMaterial = UResourceManager::GetInstance().Get<UMaterial>("M_Particle");
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
