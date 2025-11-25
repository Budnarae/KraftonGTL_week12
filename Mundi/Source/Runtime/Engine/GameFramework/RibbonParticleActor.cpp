#include "pch.h"
#include "RibbonParticleActor.h"
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
#include "../Particle/ParticleModuleTypeDataRibbon.h"
#include "Material.h"
#include "ResourceManager.h"
#include <cmath>

ARibbonParticleActor::ARibbonParticleActor()
{
	ObjectName = "Ribbon Particle Actor";
	ParticleSystemComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");
	RootComponent = ParticleSystemComponent;

	// Ribbon 파티클 시스템 설정
	{
		// 1. ParticleSystem 템플릿 생성
		UParticleSystem* RibbonTemplate = NewObject<UParticleSystem>();
		RibbonTemplate->ObjectName = "RibbonParticleSystem";

		// 2. Emitter 생성
		UParticleEmitter* RibbonEmitter = NewObject<UParticleEmitter>();
		RibbonEmitter->ObjectName = "RibbonEmitter";
		RibbonEmitter->SetMaxParticleCount(700);  // 원본 설정

		// 3. LOD Level 0에 접근
		UParticleLODLevel* LODLevel = RibbonEmitter->GetParticleLODLevelWithIndex(0);
		if (LODLevel)
		{
			LODLevel->CreateRequiredModule();

			// 4. Required Module 설정
			UParticleModuleRequired* RequiredModule = LODLevel->GetRequiredModule();
			if (RequiredModule)
			{
				// 머티리얼 설정 (Beam 셰이더 - 리본에도 사용)
				UMaterial* BaseMaterial = UResourceManager::GetInstance().Load<UMaterial>("Shaders/Effects/Beam.hlsl");
				if (BaseMaterial)
				{
					UMaterialInstanceDynamic* RibbonMaterial = UMaterialInstanceDynamic::Create(BaseMaterial);
					if (RibbonMaterial)
					{
						UTexture* RibbonTexture = UResourceManager::GetInstance().Load<UTexture>("Data/Textures/energy_beam_soft.png");
						if (RibbonTexture)
						{
							RibbonMaterial->SetTextureParameterValue(EMaterialTextureSlot::Diffuse, RibbonTexture);
						}
						RequiredModule->SetMaterial(RibbonMaterial);
					}
				}

				// 리본은 파티클이 필요하므로 SpawnRate 설정
				RequiredModule->SetSpawnRate(600.0f);  // 초당 600개 (원본 설정)
				RequiredModule->SetLifeTime(1.0f);     // 1초 수명 (원본 설정)
				RequiredModule->SetEmitterDuration(0.0f);
				RequiredModule->SetEmitterDelay(0.0f);
			}

			// 5. Location Module - 리본은 분산 없이 정확히 에미터 위치에서 스폰
			UParticleModuleLocation* LocationModule = NewObject<UParticleModuleLocation>();
			if (LocationModule)
			{
				LocationModule->SetDistributionBox(FVector::Zero(), FVector::Zero());
				LODLevel->AddSpawnModule(LocationModule);
			}

			// 6. Velocity Module - 리본은 속도 0 (에미터 이동으로 트레일 생성)
			UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
			if (VelocityModule)
			{
				VelocityModule->SetVelocityRange(FVector::Zero(), FVector::Zero());
				LODLevel->AddSpawnModule(VelocityModule);
			}

			// 7. Spawn Module
			UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
			if (SpawnModule)
			{
				SpawnModule->SetRateMin(600.0f);
				SpawnModule->SetRateMax(600.0f);
				LODLevel->AddSpawnModule(SpawnModule);
			}

			// 8. Lifetime Module
			UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
			if (LifetimeModule)
			{
				LifetimeModule->SetLifetimeRange(1.0f, 1.0f);  // 원본 설정
				LODLevel->AddSpawnModule(LifetimeModule);
			}

			// 9. Color Module
			UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
			if (ColorModule)
			{
				// 리본은 모든 파티클이 동일한 색상을 가져야 세그먼트 간 색상 차이가 없음
				// SetColorRange는 각 파티클마다 랜덤 색상을 부여하므로 SetFixedColor 사용
				ColorModule->SetFixedColor(FVector(0.5f, 0.4f, 1.0f), 1.0f);  // 파랑-보라 중간색
				LODLevel->AddSpawnModule(ColorModule);
			}

			// 10. Size Module
			UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
			if (SizeModule)
			{
				SizeModule->SetUniformSize(0.1f, 0.2f);
				LODLevel->AddSpawnModule(SizeModule);
			}

			// 11. Ribbon TypeData 설정
			UParticleModuleTypeDataRibbon* RibbonModule = NewObject<UParticleModuleTypeDataRibbon>();
			if (RibbonModule)
			{
				RibbonModule->SetRibbonWidth(0.5f);   // 적절한 폭
				RibbonModule->SetFacingMode(ERibbonFacingMode::FaceCamera);
				RibbonModule->SetMaxParticlesPerRibbon(700);  // 원본 설정

				// 테이퍼링 (꼬리가 점진적으로 가늘어짐)
				RibbonModule->SetTaperRibbon(true);
				RibbonModule->SetTaperFactor(0.1f);   // 10%까지 (덜 극단적)

				// 리본 색상 (파티클 색상에 곱해짐)
				RibbonModule->SetRibbonColor(FVector4(1.0f, 1.0f, 1.0f, 1.0f));

				// 텍스처 설정 - 단색 사용 (텍스처 경계 문제 방지)
				RibbonModule->SetTextureRepeat(1.0f);
				RibbonModule->SetUseTexture(false);

				LODLevel->SetTypeDataModule(RibbonModule);
			}
		}

		// Emitter를 ParticleSystem에 추가
		RibbonTemplate->AddEmitter(RibbonEmitter);

		// ParticleSystemComponent에 템플릿 설정
		ParticleSystemComponent->SetTemplate(RibbonTemplate);

		// 리본은 거리 기반 스폰 사용 (이동 시 촘촘한 트레일)
		ParticleSystemComponent->SetDistancePerSpawn(0.01f);

		// 파티클 시스템 활성화
		ParticleSystemComponent->Activate(false);
	}

	// MARK_AS_SPAWNABLE은 UCLASS 매크로의 DisplayName/Description으로 자동 생성됨
}

void ARibbonParticleActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 피봇 기준 궤도 회전
	if (bOrbitEnabled && DeltaTime > 0.0f)
	{
		// 첫 프레임에서 기준 위치 저장
		if (!bOrbitInitialized)
		{
			BaseLocation = GetActorLocation();
			bOrbitInitialized = true;
		}

		// 각도 업데이트
		CurrentOrbitAngle += OrbitSpeed * DeltaTime;
		if (CurrentOrbitAngle >= 360.0f)
		{
			CurrentOrbitAngle -= 360.0f;
		}

		// 라디안으로 변환
		float AngleRad = CurrentOrbitAngle * 3.14159265f / 180.0f;

		// 피봇 위치 계산
		FVector PivotLocation = BaseLocation + PivotOffset;

		// 궤도 위치 계산 (XY 평면에서 회전)
		FVector OrbitOffset;
		OrbitOffset.X = std::cos(AngleRad) * OrbitRadius;
		OrbitOffset.Y = std::sin(AngleRad) * OrbitRadius;
		OrbitOffset.Z = 0.0f;

		// 헬릭스 모드
		if (bHelixEnabled)
		{
			CurrentHelixTime += HelixSpeed * DeltaTime;
			OrbitOffset.Z = std::sin(CurrentHelixTime) * HelixAmplitude;
		}

		// 새로운 위치 설정
		FVector NewLocation = PivotLocation + OrbitOffset;
		SetActorLocation(NewLocation);
	}
}

ARibbonParticleActor::~ARibbonParticleActor()
{
}

void ARibbonParticleActor::DuplicateSubObjects()
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

void ARibbonParticleActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		ParticleSystemComponent = Cast<UParticleSystemComponent>(RootComponent);
	}
}
