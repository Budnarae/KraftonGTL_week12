#include "pch.h"
#include "PlayerController.h"
#include "Pawn.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"

APlayerController::APlayerController()
{
    ObjectName = "PlayerController";
    bCanEverTick = true;
}

APlayerController::~APlayerController() = default;

void APlayerController::BeginPlay()
{
    Super::BeginPlay();

    SetupInputComponent();
}

void APlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bInputEnabled)
    {
        return;
    }

    // Input processing happens here in the base implementation
    // Override SetupInputComponent() to bind inputs
}

void APlayerController::EndPlay()
{
    UnPossess();
    Super::EndPlay();
}

void APlayerController::Possess(APawn* InPawn)
{
    if (!InPawn)
    {
        return;
    }

    // UnPossess current pawn if any
    if (ControlledPawn)
    {
        UnPossess();
    }

    ControlledPawn = InPawn;
    ControlledPawn->PossessedBy(this);

    // Setup input on the new pawn
    if (ControlledPawn)
    {
        ControlledPawn->SetupPlayerInputComponent();
    }
}

void APlayerController::UnPossess()
{
    if (ControlledPawn)
    {
        ControlledPawn->UnPossessed();
        ControlledPawn = nullptr;
    }
}

void APlayerController::SetupInputComponent()
{
    // Base implementation does nothing
    // Override in derived classes to bind inputs
}

void APlayerController::AddYawInput(float DeltaYaw)
{
    if (PlayerCameraManager && PlayerCameraManager->GetViewCamera())
    {
        UCameraComponent* Camera = PlayerCameraManager->GetViewCamera();
        float ScaledYaw = DeltaYaw * MouseSensitivity;
        FVector CurrentRotation = Camera->GetWorldRotation().ToEulerZYXDeg();
        CurrentRotation.Z += ScaledYaw;
        Camera->SetWorldRotation(FQuat::MakeFromEulerZYX(CurrentRotation));
    }
}

void APlayerController::AddPitchInput(float DeltaPitch)
{
    if (PlayerCameraManager && PlayerCameraManager->GetViewCamera())
    {
        UCameraComponent* Camera = PlayerCameraManager->GetViewCamera();
        float ScaledPitch = DeltaPitch * MouseSensitivity * (bInvertMouseY ? 1.0f : -1.0f);
        FVector CurrentRotation = Camera->GetWorldRotation().ToEulerZYXDeg();
        CurrentRotation.X += ScaledPitch;

        // Clamp pitch to prevent camera flip
        CurrentRotation.X = FMath::Clamp(CurrentRotation.X, -89.0f, 89.0f);

        Camera->SetWorldRotation(FQuat::MakeFromEulerZYX(CurrentRotation));
    }
}
