#pragma once
#include "AnimNotify.h"
#include "USoundAnimNotify.generated.h"

class USound;

UCLASS(DisplayName="사운드 애님 노티파이", Description="애니메이션에서 사운드를 재생하는 노티파이입니다")
class USoundAnimNotify : public UAnimNotify
{
public:
    GENERATED_REFLECTION_BODY()

    USoundAnimNotify() = default;
    ~USoundAnimNotify() override = default;

    void Notify() override;
    void SerializeBinary(FArchive& Ar) override;

    AActor* GetOwner();
    void SetOwner(AActor* InOwner);

    USound* GetSound();
    void SetSound(USound* InSound);

    float GetVolume();
    void SetVolume(const float InVolume);

    float GetPitch();
    void SetPitch(const float InPitch);

private:
    AActor* Owner{};    // 사운드를 플레이할 위치를 알아야하기 때문에 소유자를 참조한다.

    UPROPERTY(EditAnywhere, Category="Sound", Tooltip="재생할 사운드 에셋")
    USound* Sound{};

    UPROPERTY(EditAnywhere, Category="Audio", Tooltip="음량 (0..1)")
    float Volume = 1.0f;

    UPROPERTY(EditAnywhere, Category="Audio", Tooltip="피치 (주파수 비율)")
    float Pitch = 1.0f;
};