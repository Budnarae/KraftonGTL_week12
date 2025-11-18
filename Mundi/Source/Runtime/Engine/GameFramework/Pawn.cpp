#include "pch.h"
#include "Pawn.h"
#include "PlayerController.h"

APawn::APawn()
{
    ObjectName = "Pawn";
    bCanEverTick = true;
}

APawn::~APawn() = default;

void APawn::BeginPlay()
{
    Super::BeginPlay();
}

void APawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void APawn::EndPlay()
{
    // UnPossess before destruction
    if (Controller)
    {
        UnPossessed();
    }

    Super::EndPlay();
}

void APawn::PossessedBy(APlayerController* InController)
{
    if (!bCanBePossessed)
    {
        return;
    }

    // UnPossess from previous controller if any
    if (Controller && Controller != InController)
    {
        UnPossessed();
    }

    Controller = InController;
}

void APawn::UnPossessed()
{
    Controller = nullptr;
}

void APawn::AddMovementInput(const FVector& WorldDirection, float ScaleValue)
{
    ControlInputVector += WorldDirection.GetNormalized() * ScaleValue;
}

FVector APawn::ConsumeMovementInputVector()
{
    LastControlInputVector = ControlInputVector;
    ControlInputVector = FVector::Zero();
    return LastControlInputVector;
}
