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

				// 빔은 파티클이 필요 없으므로 SpawnRate 0
				RequiredModule->SetSpawnRate(0.0f);  // 빔은 파티클 생성 안함
				RequiredModule->SetLifeTime(1.0f);
				RequiredModule->SetEmitterDuration(0.0f); // 0 = 무한 루프
				RequiredModule->SetEmitterDelay(0.0f);
			}

			/* 스프라이트 모듈 주석처리 - 빔 테스트용
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

			// 9. Color Module 설정 (초기 색상)
			UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
			if (ColorModule)
			{
				// 주황~노랑 색상 범위 (불꽃 느낌)
				ColorModule->SetColorRange(
					FVector(1.0f, 0.3f, 0.0f),   // Min: 주황
					FVector(1.0f, 0.8f, 0.2f)    // Max: 노랑
				);

				// 알파: 0.8~1.0 (약간 반투명~불투명)
				ColorModule->SetAlphaRange(0.8f, 1.0f);

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
			*/

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
