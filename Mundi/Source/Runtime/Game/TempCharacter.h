#pragma once
#include "Character.h"
#include "ATempCharacter.generated.h"

UCLASS(DisplayName="임시 캐릭터", Description="테스트용 캐릭터")
class ATempCharacter : public ACharacter
{
public:
    GENERATED_REFLECTION_BODY()

    ATempCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    class USpringArmComponent* SpringArm{};
    class UCameraComponent* CameraComp{};
};
