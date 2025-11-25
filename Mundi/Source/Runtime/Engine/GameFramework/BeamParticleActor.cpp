#include "pch.h"
#include "BeamParticleActor.h"
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
#include "../Particle/ParticleModuleTypeDataBeam.h"
#include "Material.h"
#include "ResourceManager.h"

ABeamParticleActor::ABeamParticleActor()
{
	ObjectName = "Beam Particle Actor";
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");
	RootComponent = ParticleSystemComponent;

	// Beam 파티클 시스템 설정
	{
		// 1. ParticleSystem 템플릿 생성
		UParticleSystem* BeamTemplate = NewObject<UParticleSystem>();
		BeamTemplate->ObjectName = "BeamParticleSystem";

		// 2. Emitter 생성
		UParticleEmitter* BeamEmitter = NewObject<UParticleEmitter>();
		BeamEmitter->ObjectName = "BeamEmitter";
		BeamEmitter->SetMaxParticleCount(100);

		// 3. LOD Level 0에 접근
		UParticleLODLevel* LODLevel = BeamEmitter->GetParticleLODLevelWithIndex(0);
		if (LODLevel)
		{
			LODLevel->CreateRequiredModule();

			// 4. Required Module 설정
			UParticleModuleRequired* RequiredModule = LODLevel->GetRequiredModule();
			if (RequiredModule)
			{
				// 머티리얼 설정 (Beam 셰이더)
				UMaterial* BaseMaterial = UResourceManager::GetInstance().Load<UMaterial>("Shaders/Effects/Beam.hlsl");
				if (BaseMaterial)
				{
					UMaterialInstanceDynamic* BeamMaterial = UMaterialInstanceDynamic::Create(BaseMaterial);
					if (BeamMaterial)
					{
						UTexture* BeamTexture = UResourceManager::GetInstance().Load<UTexture>("Data/Textures/energy_beam_soft.png");
						if (BeamTexture)
						{
							BeamMaterial->SetTextureParameterValue(EMaterialTextureSlot::Diffuse, BeamTexture);
						}
						RequiredModule->SetMaterial(BeamMaterial);
					}
				}

				RequiredModule->SetSpawnRate(1.0f);
				RequiredModule->SetLifeTime(0.0f);
				RequiredModule->SetEmitterDuration(0.0f);
				RequiredModule->SetEmitterDelay(0.0f);
			}

			// 5. Beam TypeData 설정
			UParticleModuleTypeDataBeam* BeamModule = NewObject<UParticleModuleTypeDataBeam>();
			if (BeamModule)
			{
				BeamModule->SetBeamMethod(EBeamMethod::Distance);
				BeamModule->SetBeamLength(10.0f);
				BeamModule->SetBeamWidth(0.3f);
				BeamModule->SetSegmentCount(32);
				BeamModule->SetTextureRepeat(1.0f);

				// 노이즈 설정 (번개 효과 - Midpoint Displacement 알고리즘)
				BeamModule->SetNoiseAlgorithm(EBeamNoiseAlgorithm::MidpointDisplacement);
				BeamModule->SetNoiseAmplitude(0.5f);
				BeamModule->SetNoiseFrequency(2.0f);

				// 테이퍼링
				BeamModule->SetTaperBeam(true);
				BeamModule->SetTaperFactor(0.1f);

				// 번개 색상 및 애니메이션
				BeamModule->SetBeamColor(FVector4(0.5f, 0.8f, 1.0f, 1.0f));
				BeamModule->SetJitterFrequency(15.0f);  // 번개 깜빡임 빈도
				BeamModule->SetDisplacementDecay(0.6f); // 변위 감쇠
				BeamModule->SetGlowIntensity(1.5f);

				BeamModule->SetUseTexture(true);

				LODLevel->SetTypeDataModule(BeamModule);
			}
		}

		// Emitter를 ParticleSystem에 추가
		BeamTemplate->AddEmitter(BeamEmitter);

		// ParticleSystemComponent에 템플릿 설정
		ParticleSystemComponent->SetTemplate(BeamTemplate);
		ParticleSystemComponent->SetDistancePerSpawn(0.0f);

		// 파티클 시스템 활성화
		ParticleSystemComponent->Activate(false);
	}

	// MARK_AS_SPAWNABLE은 UCLASS 매크로의 DisplayName/Description으로 자동 생성됨
}

void ABeamParticleActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

ABeamParticleActor::~ABeamParticleActor()
{
}

void ABeamParticleActor::DuplicateSubObjects()
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

void ABeamParticleActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		ParticleSystemComponent = Cast<UParticleSystemComponent>(RootComponent);
	}
}
