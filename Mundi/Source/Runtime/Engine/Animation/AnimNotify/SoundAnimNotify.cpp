#include "pch.h"
#include "SoundAnimNotify.h"

#include "FAudioDevice.h"
#include "Archive.h"
#include "ResourceManager.h"

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

void USoundAnimNotify::SerializeBinary(FArchive& Ar)
{
    Super::SerializeBinary(Ar);  // Name, TimeToNotify 처리

    if (Ar.IsSaving())
    {
        // Sound 에셋 경로 저장
        FString SoundPath = Sound ? Sound->GetFilePath() : "";
        Serialization::WriteString(Ar, SoundPath);

        // Volume, Pitch 저장
        Ar << Volume;
        Ar << Pitch;
    }
    else if (Ar.IsLoading())
    {
        // Sound 에셋 경로 로드
        FString SoundPath;
        Serialization::ReadString(Ar, SoundPath);

        if (!SoundPath.empty())
        {
            Sound = UResourceManager::GetInstance().Load<USound>(SoundPath);
        }

        // Volume, Pitch 로드
        Ar << Volume;
        Ar << Pitch;
    }
}