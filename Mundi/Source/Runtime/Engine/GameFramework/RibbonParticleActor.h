#pragma once

#include "Actor.h"
#include "ARibbonParticleActor.generated.h"

class UParticleSystemComponent;

/**
 * Ribbon 파티클 시스템 액터
 * 에디터에서 배치하면 Ribbon TypeData가 자동 설정된 파티클 시스템이 생성됩니다.
 * 에미터가 이동하면 트레일(꼬리) 효과가 나타납니다.
 * 피봇 기준 회전 모드를 활성화하면 자동으로 궤도를 그리며 리본을 생성합니다.
 */
UCLASS(DisplayName="리본 파티클", Description="리본(트레일) 이펙트를 렌더링하는 파티클 시스템 액터입니다")
class ARibbonParticleActor : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	ARibbonParticleActor();
	virtual void Tick(float DeltaTime) override;

protected:
	~ARibbonParticleActor() override;

public:
	UParticleSystemComponent* GetParticleSystemComponent() const { return ParticleSystemComponent; }

	// ───── 궤도 회전 설정 ────────────────────────────
	void SetOrbitEnabled(bool bEnabled) { bOrbitEnabled = bEnabled; }
	bool GetOrbitEnabled() const { return bOrbitEnabled; }

	void SetOrbitRadius(float Radius) { OrbitRadius = Radius; }
	float GetOrbitRadius() const { return OrbitRadius; }

	void SetOrbitSpeed(float Speed) { OrbitSpeed = Speed; }
	float GetOrbitSpeed() const { return OrbitSpeed; }

	void SetPivotOffset(const FVector& Offset) { PivotOffset = Offset; }
	FVector GetPivotOffset() const { return PivotOffset; }

	void SetHelixEnabled(bool bEnabled) { bHelixEnabled = bEnabled; }
	bool GetHelixEnabled() const { return bHelixEnabled; }

	void SetHelixAmplitude(float Amplitude) { HelixAmplitude = Amplitude; }
	float GetHelixAmplitude() const { return HelixAmplitude; }

	void SetHelixSpeed(float Speed) { HelixSpeed = Speed; }
	float GetHelixSpeed() const { return HelixSpeed; }

	// ───── 복사 관련 ────────────────────────────
	void DuplicateSubObjects() override;

	// Serialize
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UParticleSystemComponent* ParticleSystemComponent;

	// 궤도 회전 설정
	bool bOrbitEnabled = true;          // 피봇 기준 회전 활성화
	FVector PivotOffset = FVector::Zero(); // 피봇 오프셋 (액터 기준)
	float OrbitRadius = 2.0f;            // 궤도 반경
	float OrbitSpeed = 360.0f;            // 회전 속도 (도/초) - 느리게

	// 나선(헬릭스) 설정
	bool bHelixEnabled = false;          // 헬릭스 모드 활성화
	float HelixAmplitude = 1.0f;         // 헬릭스 높이 진폭
	float HelixSpeed = 2.0f;             // 헬릭스 주기 속도

	// 내부 상태
	float CurrentOrbitAngle = 0.0f;      // 현재 궤도 각도
	float CurrentHelixTime = 0.0f;       // 헬릭스 시간
	FVector BaseLocation;                // 초기 위치 (피봇 위치)
	bool bOrbitInitialized = false;      // 궤도 초기화 여부
};
