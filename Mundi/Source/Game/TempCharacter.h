#pragma once
#include "Character.h"
#include "ATempCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS(DisplayName="임시 캐릭터", Description="테스트용 캐릭터")
class ATempCharacter : public ACharacter
{
public:
    GENERATED_REFLECTION_BODY()

    ATempCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    USpringArmComponent* SpringArm = nullptr;
    UCameraComponent* CameraComp = nullptr;

    // 카메라 회전
    float CameraYaw = 0.0f;
    float CameraPitch = 0.0f;
    float MouseSensitivity = 0.1f;

    // 캐릭터 회전 속도 (degrees per second)
    float RotationSpeed = 720.0f;
};
