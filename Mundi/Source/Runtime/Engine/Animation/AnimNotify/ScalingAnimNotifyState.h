#pragma once
#include "AnimNotifyState.h"
#include "UScalingAnimNotifyState.generated.h"

class AActor;
struct FVector;

UCLASS(DisplayName="스케일 애님 노티파이 스테이트", Description="지속 시간 동안 액터를 스케일링하는 노티파이 스테이트입니다")
class UScalingAnimNotifyState : public UAnimNotifyState
{
public:
    GENERATED_REFLECTION_BODY()

    UScalingAnimNotifyState() = default;
    ~UScalingAnimNotifyState() override = default;

    void NotifyBegin() override;
    void NotifyTick() override;
    void NotifyEnd() override;

    AActor* GetOwner();
    void SetOwner(AActor* InOwner);

    FVector GetScalePerTick();
    void SetScalePerTick(const FVector& InScale);

    bool GetRollbackOnEnd();
    void SetRollbackOnEnd(bool bInRollback);

private:
    AActor* Owner{};    // 스케일링할 액터

    UPROPERTY(EditAnywhere, Category="Scale", Tooltip="매 틱마다 추가할 스케일 (X, Y, Z)")
    FVector ScalePerTick{};

    UPROPERTY(EditAnywhere, Category="Scale", Tooltip="종료 시 초기 스케일값으로 복원할지 여부")
    bool bRollbackOnEnd = false;

    // 초기 스케일값 저장
    FVector InitialScale{};
};
