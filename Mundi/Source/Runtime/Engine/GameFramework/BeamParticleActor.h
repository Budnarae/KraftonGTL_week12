#pragma once

#include "Actor.h"
#include "ABeamParticleActor.generated.h"

class UParticleSystemComponent;

/**
 * Beam 파티클 시스템 액터
 * 에디터에서 배치하면 Beam TypeData가 자동 설정된 파티클 시스템이 생성됩니다.
 */
UCLASS(DisplayName="빔 파티클", Description="빔 이펙트를 렌더링하는 파티클 시스템 액터입니다")
class ABeamParticleActor : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	ABeamParticleActor();
	virtual void Tick(float DeltaTime) override;

protected:
	~ABeamParticleActor() override;

public:
	UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }

	// ───── 복사 관련 ────────────────────────────
	void DuplicateSubObjects() override;

	// Serialize
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UParticleSystemComponent* ParticleSystemComponent;
};
