#include "pch.h"
#include "SoundAnimNotify.h"

#include "FAudioDevice.h"

IMPLEMENT_CLASS(USoundAnimNotify)

void USoundAnimNotify::Notify()
{
    if (!Sound)
    {
        UE_LOG("[SoundAnimNotify] Sound is null, skipping notify");
        return;
    }

    if (!Owner)
    {
        UE_LOG("[SoundAnimNotify] Owner is null, skipping notify");
        return;
    }

    if (Owner->IsPendingDestroy())
    {
        UE_LOG("[SoundAnimNotify] Owner is pending destroy, skipping notify");
        return;
    }

    FVector SoundPlayLocation = Owner->GetActorLocation();
    IXAudio2SourceVoice* SourceVoice = FAudioDevice::PlaySound3D(Sound, SoundPlayLocation, Volume, false);
    if (SourceVoice)
    {
        SourceVoice->SetFrequencyRatio(Pitch);
    }
}

AActor* USoundAnimNotify::GetOwner()
{
    return Owner;
}

void USoundAnimNotify::SetOwner(AActor* InOwner)
{
    Owner = InOwner;
}

USound* USoundAnimNotify::GetSound()
{
    return Sound;
}

void USoundAnimNotify::SetSound(USound* InSound)
{
    Sound = InSound;
}

float USoundAnimNotify::GetVolume()
{
    return Volume;
}

void USoundAnimNotify::SetVolume(const float InVolume)
{
    Volume = InVolume;
}

float USoundAnimNotify::GetPitch()
{
    return Pitch;
}

void USoundAnimNotify::SetPitch(const float InPitch)
{
    Pitch = InPitch;
}