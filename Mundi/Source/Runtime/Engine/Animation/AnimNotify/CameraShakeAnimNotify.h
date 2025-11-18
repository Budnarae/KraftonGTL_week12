#pragma once
#include "AnimNotify.h"
#include "UCameraShakeAnimNotify.generated.h"

UCLASS(DisplayName="카메라 쉐이크 애님 노티파이", Description="애니메이션에서 카메라 쉐이크를 트리거하는 노티파이입니다")
class UCameraShakeAnimNotify : public UAnimNotify
{
public:
	GENERATED_REFLECTION_BODY()

	UCameraShakeAnimNotify() = default;
	~UCameraShakeAnimNotify() override = default;

	void Notify() override;

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
};
