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

	// 기본 파티클 시스템은 빈 상태로 생성됩니다.
	// 에디터에서 파티클 에셋을 지정하거나, Beam/Ribbon 전용 액터를 사용하세요.
	// - ABeamParticleActor: 빔 이펙트
	// - ARibbonParticleActor: 리본(트레일) 이펙트
}

void AParticleSystemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Beam Target Test Code - 웬만하면 삭제하지 마세요
	//if (ParticleSystemComponent && !ParticleSystemComponent->GetBeamTargetActor())
	//{
	//	if (UWorld* World = GetWorld())
	//	{
	//		// "TestActor_1" 이름을 가진 액터를 찾아서 타겟으로 설정
	//		AActor* TestActor = World->FindActorByName(FName("ATempCharacter_1"));
	//		if (TestActor)
	//		{
	//			ParticleSystemComponent->SetBeamTargetActor(TestActor);
	//		}
	//	}
	//}
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
