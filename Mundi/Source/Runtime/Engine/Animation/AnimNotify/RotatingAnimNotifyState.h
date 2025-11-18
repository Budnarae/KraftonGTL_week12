#pragma once
#include "AnimNotifyState.h"
#include "URotatingAnimNotifyState.generated.h"

class AActor;
struct FVector;
struct FQuat;

UCLASS(DisplayName="회전 애님 노티파이 스테이트", Description="지속 시간 동안 액터를 회전시키는 노티파이 스테이트입니다")
class URotatingAnimNotifyState : public UAnimNotifyState
{
public:
    GENERATED_REFLECTION_BODY()

    URotatingAnimNotifyState() = default;
    ~URotatingAnimNotifyState() override = default;

    void NotifyBegin() override;
    void NotifyTick() override;
    void NotifyEnd() override;

    AActor* GetOwner();
    void SetOwner(AActor* InOwner);

    FVector GetRotationPerTick();
    void SetRotationPerTick(const FVector& InRotation);

    bool GetRollbackOnEnd();
    void SetRollbackOnEnd(bool bInRollback);

private:
    AActor* Owner{};    // 회전시킬 액터

    UPROPERTY(EditAnywhere, Category="Rotation", Tooltip="매 틱마다 회전할 각도 (Roll, Pitch, Yaw) - Degrees")
    FVector RotationPerTick{};

    UPROPERTY(EditAnywhere, Category="Rotation", Tooltip="종료 시 초기 회전값으로 복원할지 여부")
    bool bRollbackOnEnd = false;

    // 초기 회전값 저장 (Quaternion)
    FQuat InitialRotation{};
};
