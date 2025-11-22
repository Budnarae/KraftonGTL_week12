#pragma once

#include "Actor.h"
#include "AParticleSystemActor.generated.h"

class UParticleSystemComponent;

UCLASS(DisplayName="파티클 시스템", Description="파티클 시스템을 배치하는 액터입니다")
class AParticleSystemActor : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	AParticleSystemActor();
	virtual void Tick(float DeltaTime) override;
protected:
	~AParticleSystemActor() override;

public:
	UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }
	void SetParticleSystemComponent(UParticleSystemComponent* InParticleSystemComponent);

	// ───── 복사 관련 ────────────────────────────
	void DuplicateSubObjects() override;

	// Serialize
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UParticleSystemComponent* ParticleSystemComponent;
};
