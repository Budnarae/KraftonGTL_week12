#include "pch.h"
#include "SoundAnimNotify.h"

#include "FAudioDevice.h"

IMPLEMENT_CLASS(USoundAnimNotify)

void USoundAnimNotify::Notify()
{
    if (!Sound || !Owner/* || Owner->IsPendingDestroy()*/)
        return;

    // Crash 방지용 임시
    // FVector SoundPlayLocation = Owner->GetActorLocation();

    FVector SoundPlayLocation{};
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