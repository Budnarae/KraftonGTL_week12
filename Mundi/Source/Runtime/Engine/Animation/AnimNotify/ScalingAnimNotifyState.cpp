#include "pch.h"
#include "ScalingAnimNotifyState.h"
#include "Actor.h"

IMPLEMENT_CLASS(UScalingAnimNotifyState)

void UScalingAnimNotifyState::NotifyBegin()
{
    UAnimNotifyState::NotifyBegin();

    UE_LOG("[ScalingAnimNotifyState] NotifyBegin: Scaling started");
}

void UScalingAnimNotifyState::NotifyTick()
{
    if (!Owner)
        return;

    // 현재 스케일값에 ScalePerTick 만큼 추가
    FVector CurrentScale = Owner->GetActorScale();
    FVector NewScale = CurrentScale + ScalePerTick;
    Owner->SetActorScale(NewScale);
}

void UScalingAnimNotifyState::NotifyEnd()
{
    UAnimNotifyState::NotifyEnd();

    if (bRollbackOnEnd && Owner)
    {
        // 초기 스케일값으로 복원
        Owner->SetActorScale(InitialScale);
        UE_LOG("[ScalingAnimNotifyState] NotifyEnd: Scale rolled back to initial value");
    }
    else
    {
        UE_LOG("[ScalingAnimNotifyState] NotifyEnd: Scaling finished without rollback");
    }
}

AActor* UScalingAnimNotifyState::GetOwner()
{
    return Owner;
}

void UScalingAnimNotifyState::SetOwner(AActor* InOwner)
{
    Owner = InOwner;

    if (Owner)
    {
        // 초기 스케일값 저장
        InitialScale = Owner->GetActorScale();
    }
}

FVector UScalingAnimNotifyState::GetScalePerTick()
{
    return ScalePerTick;
}

void UScalingAnimNotifyState::SetScalePerTick(const FVector& InScale)
{
    ScalePerTick = InScale;
}

bool UScalingAnimNotifyState::GetRollbackOnEnd()
{
    return bRollbackOnEnd;
}

void UScalingAnimNotifyState::SetRollbackOnEnd(bool bInRollback)
{
    bRollbackOnEnd = bInRollback;
}
