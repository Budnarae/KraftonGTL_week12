#include "pch.h"
#include "CameraShakeAnimNotify.h"
#include "World.h"
#include "PlayerCameraManager.h"

IMPLEMENT_CLASS(UCameraShakeAnimNotify)

void UCameraShakeAnimNotify::Notify()
{
	if (!GWorld) return;

	APlayerCameraManager* CameraManager = GWorld->GetPlayerCameraManager();
	if (!CameraManager) return;

	CameraManager->StartCameraShake(Duration, AmplitudeLocation, AmplitudeRotationDeg, Frequency, Priority);
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
