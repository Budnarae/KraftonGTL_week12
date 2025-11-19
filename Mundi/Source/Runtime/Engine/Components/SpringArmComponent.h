#pragma once

#include "SceneComponent.h"
#include "USpringArmComponent.generated.h"

/**
 * @class USpringArmComponent
 * @brief 카메라를 타겟으로부터 일정 거리에 유지하는 스프링 암 컴포넌트
 *
 * 자식 컴포넌트(보통 카메라)를 이 컴포넌트의 뒤쪽으로 TargetArmLength만큼 떨어진 위치에 배치합니다.
 */
UCLASS(DisplayName="스프링 암", Description="카메라를 타겟으로부터 일정 거리에 유지하는 컴포넌트")
class USpringArmComponent : public USceneComponent
{
public:
    GENERATED_REFLECTION_BODY()

    USpringArmComponent();
    virtual ~USpringArmComponent() = default;

    virtual void TickComponent(float DeltaTime) override;

    // 프로퍼티 Getter/Setter
    float GetTargetArmLength() const { return TargetArmLength; }
    void SetTargetArmLength(float InLength) { TargetArmLength = InLength; }

    FVector GetSocketOffset() const { return SocketOffset; }
    void SetSocketOffset(const FVector& InOffset) { SocketOffset = InOffset; }

    FVector GetTargetOffset() const { return TargetOffset; }
    void SetTargetOffset(const FVector& InOffset) { TargetOffset = InOffset; }

    bool IsUsingPawnControlRotation() const { return bUsePawnControlRotation; }
    void SetUsePawnControlRotation(bool bUse) { bUsePawnControlRotation = bUse; }

    // 현재 암 위치 (자식 컴포넌트가 배치될 위치)
    FVector GetArmLocation() const;
    FQuat GetArmRotation() const;

protected:
    // 자식 컴포넌트들의 위치를 업데이트
    void UpdateChildTransforms();

public:
    // 스프링 암 길이 (타겟으로부터 카메라까지의 거리)
    UPROPERTY(EditAnywhere, Category="SpringArm", Tooltip="스프링 암의 길이입니다.")
    float TargetArmLength = 3.0f;

    // 소켓 오프셋 (암 끝에서의 추가 오프셋)
    UPROPERTY(EditAnywhere, Category="SpringArm", Tooltip="암 끝에서의 추가 오프셋입니다.")
    FVector SocketOffset{};

    // 타겟 오프셋 (암 시작점에서의 오프셋)
    UPROPERTY(EditAnywhere, Category="SpringArm", Tooltip="암 시작점에서의 오프셋입니다.")
    FVector TargetOffset{};

    // Pawn의 컨트롤 회전을 사용할지 여부
    UPROPERTY(EditAnywhere, Category="SpringArm", Tooltip="Pawn의 컨트롤 회전을 사용합니다.")
    bool bUsePawnControlRotation = false;

    // 충돌 테스트 활성화 (향후 구현)
    UPROPERTY(EditAnywhere, Category="SpringArm|Collision", Tooltip="충돌 테스트를 수행합니다.")
    bool bDoCollisionTest = false;

    // 카메라 Lag 활성화
    UPROPERTY(EditAnywhere, Category="SpringArm|Lag", Tooltip="카메라 위치 보간을 활성화합니다.")
    bool bEnableCameraLag = false;

    // 카메라 Lag 속도
    UPROPERTY(EditAnywhere, Category="SpringArm|Lag", Tooltip="카메라 위치 보간 속도입니다.")
    float CameraLagSpeed = 10.0f;

    // 카메라 회전 Lag 활성화
    UPROPERTY(EditAnywhere, Category="SpringArm|Lag", Tooltip="카메라 회전 보간을 활성화합니다.")
    bool bEnableCameraRotationLag = false;

    // 카메라 회전 Lag 속도
    UPROPERTY(EditAnywhere, Category="SpringArm|Lag", Tooltip="카메라 회전 보간 속도입니다.")
    float CameraRotationLagSpeed = 10.0f;

private:
    // Lag 계산을 위한 이전 위치/회전
    FVector PreviousArmLocation;
    FQuat PreviousArmRotation;
    bool bFirstTick = true;
};
