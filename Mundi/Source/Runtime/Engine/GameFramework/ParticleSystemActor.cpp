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
#include "../Particle/ParticleModuleTypeDataBeam.h"
#include "../Particle/ParticleModuleTypeDataRibbon.h"
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
		TestEmitter->SetMaxParticleCount(1000);  // SpawnRate(600) * Lifetime(1) 이상 필요

		// 3. LOD Level 0에 접근 (이미 생성되어 있음)
		UParticleLODLevel* TestLODLevel = TestEmitter->GetParticleLODLevelWithIndex(0);
		if (TestLODLevel)
		{
			TestLODLevel->CreateRequiredModule();

			// 4. Required Module 설정 (필수 모듈)
			UParticleModuleRequired* RequiredModule = TestLODLevel->GetRequiredModule();
			if (RequiredModule)
			{
				// 머티리얼 설정 (Beam 셰이더 사용 - 빌보드 회전 없음)
				UMaterial* BaseMaterial = UResourceManager::GetInstance().Load<UMaterial>("Shaders/Effects/Beam.hlsl");
				if (BaseMaterial)
				{
					// 동적 머티리얼 인스턴스 생성
					UMaterialInstanceDynamic* ParticleMaterial = UMaterialInstanceDynamic::Create(BaseMaterial);
					if (ParticleMaterial)
					{
						// 텍스처 로드 및 설정
						//UTexture* BeamTexture = UResourceManager::GetInstance().Load<UTexture>("Data/Textures/lightning_beam_01.png");
						UTexture* BeamTexture = UResourceManager::GetInstance().Load<UTexture>("Data/Textures/energy_beam_soft.png");
						if (BeamTexture)
						{
							ParticleMaterial->SetTextureParameterValue(EMaterialTextureSlot::Diffuse, BeamTexture);
						}

						RequiredModule->SetMaterial(ParticleMaterial);
					}
				}

				// 리본은 파티클이 필요하므로 SpawnRate 설정
				RequiredModule->SetSpawnRate(600.0f);  // 초당 60개 (부드러운 리본)
				RequiredModule->SetLifeTime(1.0f);
				RequiredModule->SetEmitterDuration(0.0f); // 0 = 무한 루프
				RequiredModule->SetEmitterDelay(0.0f);
			}

			// 5. Location Module 설정 (리본은 분산 없이 정확히 에미터 위치에서 스폰)
			UParticleModuleLocation* LocationModule = NewObject<UParticleModuleLocation>();
			if (LocationModule)
			{
				// 리본: 분산 0 - 모든 파티클이 에미터 위치에서 스폰되어야 경로가 리본이 됨
				LocationModule->SetDistributionBox(FVector::Zero(), FVector::Zero());
				TestLODLevel->AddSpawnModule(LocationModule);
			}

			// 6. Velocity Module 설정 (리본은 속도 0 - 에미터 이동으로 트레일 생성)
			UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
			if (VelocityModule)
			{
				// 리본: 파티클이 제자리에 있어야 에미터 경로가 트레일이 됨
				VelocityModule->SetVelocityRange(
					FVector(0.0f, 0.0f, 0.0f),  // Min
					FVector(0.0f, 0.0f, 0.0f)   // Max
				);

				TestLODLevel->AddSpawnModule(VelocityModule);
			}

			// 7. Spawn Module 설정 (스폰율)
			UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
			if (SpawnModule)
			{
				// 기본 스폰율: 초당 60개 (부드러운 리본을 위해 촘촘하게)
				SpawnModule->SetRateMin(600.0f);
				SpawnModule->SetRateMax(600.0f);

				TestLODLevel->AddSpawnModule(SpawnModule);
			}

			// 8. Lifetime Module 설정 (파티클 수명)
			UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
			if (LifetimeModule)
			{
				// 파티클 수명: 1초 (60개 포인트로 리본 형성)
				LifetimeModule->SetLifetimeRange(1.0f, 1.0f);

				TestLODLevel->AddSpawnModule(LifetimeModule);
			}

			// 9. Color Module 설정 (초기 색상)
			UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
			if (ColorModule)
			{
				// 파란~보라 색상 범위 (트레일 느낌)
				ColorModule->SetColorRange(
					FVector(0.2f, 0.5f, 1.0f),   // Min: 파랑
					FVector(0.8f, 0.3f, 1.0f)    // Max: 보라
				);

				// 알파: 0.8~1.0
				ColorModule->SetAlphaRange(0.8f, 1.0f);

				TestLODLevel->AddSpawnModule(ColorModule);
			}

			// 10. Size Module 설정 (초기 크기 - 리본에서는 사용 안함)
			UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
			if (SizeModule)
			{
				SizeModule->SetUniformSize(0.1f, 0.2f);
				TestLODLevel->AddSpawnModule(SizeModule);
			}

			// 11. Ribbon TypeData 설정 (리본 렌더링 테스트)
			UParticleModuleTypeDataRibbon* RibbonModule = NewObject<UParticleModuleTypeDataRibbon>();
			if (RibbonModule)
			{
				// 리본 기본 설정
				RibbonModule->SetRibbonWidth(1.5f);                // 리본 너비 (더 굵게)
				RibbonModule->SetFacingMode(ERibbonFacingMode::FaceCamera);  // 카메라 향함
				RibbonModule->SetMaxParticlesPerRibbon(700);       // SpawnRate * Lifetime 이상 (600개 + 여유)

				// 테이퍼링 (꼬리가 가늘어짐)
				RibbonModule->SetTaperRibbon(true);
				RibbonModule->SetTaperFactor(0.05f);               // 꼬리 5% 두께

				// 리본 색상 (파티클 색상에 곱해짐)
				RibbonModule->SetRibbonColor(FVector4(1.0f, 1.0f, 1.0f, 1.0f));

				// 텍스처 설정
				RibbonModule->SetTextureRepeat(3.0f);              // 텍스처 3번 반복
				RibbonModule->SetUseTexture(false);                // 단색 테스트

				TestLODLevel->SetTypeDataModule(RibbonModule);
			}

			/* Beam TypeData 주석처리 - 리본 테스트용
			// 11. Beam TypeData 설정 (빔 렌더링 테스트)
			UParticleModuleTypeDataBeam* BeamModule = NewObject<UParticleModuleTypeDataBeam>();
			if (BeamModule)
			{
				// 빔 기본 설정
				BeamModule->SetBeamMethod(EBeamMethod::Distance);  // 거리 기반 빔
				BeamModule->SetBeamLength(20.0f);                  // 10 유닛 길이
				BeamModule->SetBeamWidth(0.2f);                   // 빔 너비
				BeamModule->SetSegmentCount(60);                   // 32개 세그먼트 (많은 꺾임)
				BeamModule->SetTextureRepeat(1.0f);                // 텍스처 1번 반복

				// 노이즈 설정 (각진 번개 효과)
				BeamModule->SetNoiseAmplitude(2.0f);               // 노이즈 강도
				BeamModule->SetNoiseFrequency(1.0f);

				// 테이퍼링 (양 끝이 얇고 중간이 두꺼움)
				BeamModule->SetTaperBeam(true);
				BeamModule->SetTaperFactor(0.05f);                  // 끝단 5% 두께

				// 새로운 번개 파라미터
				BeamModule->SetBeamColor(FVector4(1.0f, 1.0f, 1.0f, 1.0f));  // 흰색 (텍스처 틴트)
				BeamModule->SetJitterFrequency(30.0f);             // 초당 30번 지직거림
				BeamModule->SetDisplacementDecay(0.6f);            // 변위 감소율
				BeamModule->SetGlowIntensity(1.5f);                // 밝기 1.5배

				// 텍스처 사용 활성화
				BeamModule->SetUseTexture(true);

				TestLODLevel->SetTypeDataModule(BeamModule);
			}
			*/
		}

		// Emitter를 ParticleSystem에 추가
		TestTemplate->AddEmitter(TestEmitter);

		// ParticleSystemComponent에 템플릿 설정
		ParticleSystemComponent->SetTemplate(TestTemplate);

		// 거리 기반 스폰 설정 (0.1 유닛마다 1개 파티클 스폰)
		// 0이면 시간 기반 스폰 사용
		ParticleSystemComponent->SetDistancePerSpawn(0.05f);

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

	// 플레이어에 어태치하거나 외부에서 위치를 제어할 때는 아래 코드가 필요 없음
	// 아래는 독립 테스트용 원형 이동 코드 (필요시 주석 해제)
	/*
	static float ElapsedTime = 0.0f;
	ElapsedTime += DeltaTime;

	float Radius = 5.0f;
	float Speed = 2.0f;
	FVector NewLocation;
	NewLocation.X = Radius * cosf(ElapsedTime * Speed);
	NewLocation.Y = Radius * sinf(ElapsedTime * Speed);
	NewLocation.Z = 2.0f + sinf(ElapsedTime * Speed * 0.5f) * 1.0f;

	SetActorLocation(NewLocation);
	*/
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
