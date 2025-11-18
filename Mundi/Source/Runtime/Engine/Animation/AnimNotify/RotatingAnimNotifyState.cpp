#include "pch.h"
#include "RotatingAnimNotifyState.h"
#include "Actor.h"

IMPLEMENT_CLASS(URotatingAnimNotifyState)

void URotatingAnimNotifyState::NotifyBegin()
{
    UAnimNotifyState::NotifyBegin();

    UE_LOG("[RotatingAnimNotifyState] NotifyBegin: Rotation started");

    if (Owner)
    {
        // 초기 회전값 저장 (Quaternion)
        InitialRotation = Owner->GetActorRotation();
    }
}

void URotatingAnimNotifyState::NotifyTick()
{
    if (!Owner)
        return;

    // 현재 회전값을 Euler angles로 변환
    FQuat CurrentRotationQuat = Owner->GetActorRotation();
    FVector CurrentRotationEuler = CurrentRotationQuat.ToEulerZYXDeg();

    // RotationPerTick 만큼 추가
    FVector NewRotationEuler = CurrentRotationEuler + RotationPerTick;

    // 다시 설정 (SetActorRotation은 FVector Euler를 받음)
    Owner->SetActorRotation(NewRotationEuler);
}

void URotatingAnimNotifyState::NotifyEnd()
{
    UAnimNotifyState::NotifyEnd();

    if (bRollbackOnEnd && Owner)
    {
        // 초기 회전값으로 복원
        Owner->SetActorRotation(InitialRotation);
        UE_LOG("[RotatingAnimNotifyState] NotifyEnd: Rotation rolled back to initial value");
    }
    else
    {
        UE_LOG("[RotatingAnimNotifyState] NotifyEnd: Rotation finished without rollback");
    }
}

AActor* URotatingAnimNotifyState::GetOwner()
{
    return Owner;
}

void URotatingAnimNotifyState::SetOwner(AActor* InOwner)
{
    Owner = InOwner;
}

FVector URotatingAnimNotifyState::GetRotationPerTick()
{
    return RotationPerTick;
}

void URotatingAnimNotifyState::SetRotationPerTick(const FVector& InRotation)
{
    RotationPerTick = InRotation;
}

bool URotatingAnimNotifyState::GetRollbackOnEnd()
{
    return bRollbackOnEnd;
}

void URotatingAnimNotifyState::SetRollbackOnEnd(bool bInRollback)
{
    bRollbackOnEnd = bInRollback;
}
