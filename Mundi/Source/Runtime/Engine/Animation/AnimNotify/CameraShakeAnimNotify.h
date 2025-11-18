#pragma once
#include "AnimNotify.h"
#include "../GameFramework/Camera/CamMod_Shake.h"
#include "UCameraShakeAnimNotify.generated.h"

UCLASS(DisplayName="카메라 쉐이크 애님 노티파이", Description="애니메이션에서 카메라 쉐이크를 트리거하는 노티파이입니다")
class UCameraShakeAnimNotify : public UAnimNotify
{
public:
	GENERATED_REFLECTION_BODY()

	UCameraShakeAnimNotify() = default;
	~UCameraShakeAnimNotify() override = default;

	void Notify() override;
	void SerializeBinary(FArchive& Ar) override;

	float GetDuration();
	void SetDuration(const float InDuration);

	float GetAmplitudeLocation();
	void SetAmplitudeLocation(const float InAmplitudeLocation);

	float GetAmplitudeRotationDeg();
	void SetAmplitudeRotationDeg(const float InAmplitudeRotationDeg);

	float GetFrequency();
	void SetFrequency(const float InFrequency);

	int32 GetPriority();
	void SetPriority(const int32 InPriority);

	EShakeNoise GetNoiseMode();
	void SetNoiseMode(const EShakeNoise InNoiseMode);

	float GetMixRatio();
	void SetMixRatio(const float InMixRatio);

private:
	UPROPERTY(EditAnywhere, Category="CameraShake", Tooltip="쉐이크 지속 시간 (초)")
	float Duration = 0.5f;

	UPROPERTY(EditAnywhere, Category="CameraShake", Tooltip="위치 흔들림 진폭")
	float AmplitudeLocation = 10.0f;

	UPROPERTY(EditAnywhere, Category="CameraShake", Tooltip="회전 흔들림 진폭 (도)")
	float AmplitudeRotationDeg = 2.0f;

	UPROPERTY(EditAnywhere, Category="CameraShake", Tooltip="쉐이크 주파수")
	float Frequency = 20.0f;

	UPROPERTY(EditAnywhere, Category="CameraShake", Tooltip="우선순위")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, Category="CameraShake", Tooltip="노이즈 모드 (Sine/Perlin/Mixed)")
	EShakeNoise NoiseMode = EShakeNoise::Sine;

	UPROPERTY(EditAnywhere, Category="CameraShake", Tooltip="Mixed 모드 시 혼합 비율 (0=Sine, 1=Perlin)")
	float MixRatio = 0.5f;
};
