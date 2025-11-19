#include "pch.h"
#include "SpringArmComponent.h"

USpringArmComponent::USpringArmComponent()
{
    bCanEverTick = true;
}

void USpringArmComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    // 자식 컴포넌트 위치 업데이트
    UpdateChildTransforms();
}

FVector USpringArmComponent::GetArmLocation() const
{
    // 기본 위치: 이 컴포넌트의 월드 위치 + 타겟 오프셋
    FVector BaseLocation = GetWorldLocation() + GetWorldRotation().RotateVector(TargetOffset);

    // 암 방향: 이 컴포넌트의 후방 (로컬 -X)
    FVector ArmDirection = GetWorldRotation().RotateVector(FVector(-1, 0, 0));

    // 암 끝 위치
    FVector ArmEndLocation = BaseLocation + ArmDirection * TargetArmLength;

    // 소켓 오프셋 적용
    ArmEndLocation += GetWorldRotation().RotateVector(SocketOffset);

    return ArmEndLocation;
}

FQuat USpringArmComponent::GetArmRotation() const
{
    return GetWorldRotation();
}

void USpringArmComponent::UpdateChildTransforms()
{
    FVector DesiredLocation = GetArmLocation();
    FQuat DesiredRotation = GetArmRotation();

    // Lag 처리
    if (bFirstTick)
    {
        PreviousArmLocation = DesiredLocation;
        PreviousArmRotation = DesiredRotation;
        bFirstTick = false;
    }

    FVector FinalLocation = DesiredLocation;
    FQuat FinalRotation = DesiredRotation;

    // 위치 Lag
    if (bEnableCameraLag && CameraLagSpeed > 0.0f)
    {
        float DeltaTime = GetWorld() ? GetWorld()->GetDeltaTime(EDeltaTime::Unscaled) : 0.016f;
        float LagAlpha = FMath::Clamp(DeltaTime * CameraLagSpeed, 0.0f, 1.0f);
        FinalLocation = FVector::Lerp(PreviousArmLocation, DesiredLocation, LagAlpha);
    }

    // 회전 Lag
    if (bEnableCameraRotationLag && CameraRotationLagSpeed > 0.0f)
    {
        float DeltaTime = GetWorld() ? GetWorld()->GetDeltaTime(EDeltaTime::Unscaled) : 0.016f;
        float LagAlpha = FMath::Clamp(DeltaTime * CameraRotationLagSpeed, 0.0f, 1.0f);
        FinalRotation = FQuat::Slerp(PreviousArmRotation, DesiredRotation, LagAlpha);
    }

    PreviousArmLocation = FinalLocation;
    PreviousArmRotation = FinalRotation;

    // 모든 자식 컴포넌트 업데이트
    for (USceneComponent* Child : GetAttachChildren())
    {
        if (Child)
        {
            // 자식의 상대 위치/회전을 무시하고 암 위치에 직접 배치
            Child->SetWorldLocation(FinalLocation);
            Child->SetWorldRotation(FinalRotation);
        }
    }
}
