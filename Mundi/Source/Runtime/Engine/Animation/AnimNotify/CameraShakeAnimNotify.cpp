#include "pch.h"
#include "CameraShakeAnimNotify.h"
#include "World.h"
#include "PlayerCameraManager.h"
#include "../GameFramework/Camera/CamMod_Shake.h"
#include "Archive.h"

IMPLEMENT_CLASS(UCameraShakeAnimNotify)

void UCameraShakeAnimNotify::Notify()
{
	if (!GWorld) return;

	APlayerCameraManager* CameraManager = GWorld->GetPlayerCameraManager();
	if (!CameraManager) return;

	// UCamMod_Shake를 직접 생성하고 설정 (기존 프레임워크 방식)
	UCamMod_Shake* ShakeModifier = new UCamMod_Shake();
	ShakeModifier->Priority = Priority;
	ShakeModifier->NoiseMode = NoiseMode;
	ShakeModifier->MixRatio = MixRatio;
	ShakeModifier->Initialize(Duration, AmplitudeLocation, AmplitudeRotationDeg, Frequency);
	CameraManager->ActiveModifiers.Add(ShakeModifier);
}

float UCameraShakeAnimNotify::GetDuration()
{
	return Duration;
}

void UCameraShakeAnimNotify::SetDuration(const float InDuration)
{
	Duration = InDuration;
}

float UCameraShakeAnimNotify::GetAmplitudeLocation()
{
	return AmplitudeLocation;
}

void UCameraShakeAnimNotify::SetAmplitudeLocation(const float InAmplitudeLocation)
{
	AmplitudeLocation = InAmplitudeLocation;
}

float UCameraShakeAnimNotify::GetAmplitudeRotationDeg()
{
	return AmplitudeRotationDeg;
}

void UCameraShakeAnimNotify::SetAmplitudeRotationDeg(const float InAmplitudeRotationDeg)
{
	AmplitudeRotationDeg = InAmplitudeRotationDeg;
}

float UCameraShakeAnimNotify::GetFrequency()
{
	return Frequency;
}

void UCameraShakeAnimNotify::SetFrequency(const float InFrequency)
{
	Frequency = InFrequency;
}

int32 UCameraShakeAnimNotify::GetPriority()
{
	return Priority;
}

void UCameraShakeAnimNotify::SetPriority(const int32 InPriority)
{
	Priority = InPriority;
}

EShakeNoise UCameraShakeAnimNotify::GetNoiseMode()
{
	return NoiseMode;
}

void UCameraShakeAnimNotify::SetNoiseMode(const EShakeNoise InNoiseMode)
{
	NoiseMode = InNoiseMode;
}

float UCameraShakeAnimNotify::GetMixRatio()
{
	return MixRatio;
}

void UCameraShakeAnimNotify::SetMixRatio(const float InMixRatio)
{
	MixRatio = InMixRatio;
}

void UCameraShakeAnimNotify::SerializeBinary(FArchive& Ar)
{
	Super::SerializeBinary(Ar);  // Name, TimeToNotify 처리

	if (Ar.IsSaving())
	{
		Ar << Duration;
		Ar << AmplitudeLocation;
		Ar << AmplitudeRotationDeg;
		Ar << Frequency;
		Ar << Priority;

		int32 NoiseModeInt = static_cast<int32>(NoiseMode);
		Ar << NoiseModeInt;

		Ar << MixRatio;
	}
	else if (Ar.IsLoading())
	{
		Ar << Duration;
		Ar << AmplitudeLocation;
		Ar << AmplitudeRotationDeg;
		Ar << Frequency;
		Ar << Priority;

		int32 NoiseModeInt;
		Ar << NoiseModeInt;
		NoiseMode = static_cast<EShakeNoise>(NoiseModeInt);

		Ar << MixRatio;
	}
}
