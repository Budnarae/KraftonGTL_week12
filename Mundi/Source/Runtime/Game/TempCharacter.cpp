#include "pch.h"
#include "TempCharacter.h"
#include "PlayerController.h"
#include "PlayerCameraManager.h"
#include "SpringArmComponent.h"
#include "InputManager.h"

ATempCharacter::ATempCharacter()
{
    ObjectName = "TempCharacter";

    // 스켈레탈 메시 설정
    if (MeshComponent)
    {
        MeshComponent->SetSkeletalMesh("Content/Resources/James/James");
    }

    SpringArm = CreateDefaultSubobject<USpringArmComponent>("Spring Arm");
    SpringArm->SetRelativeLocation({0, 0, 1.5f});
    SpringArm->SetupAttachment(RootComponent, EAttachmentRule::KeepRelative);
    
    CameraComp = CreateDefaultSubobject<UCameraComponent>("DefaultCamera");
    CameraComp->SetupAttachment(SpringArm, EAttachmentRule::KeepRelative);
}

void ATempCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (CameraComp)
    {
        if (APlayerCameraManager* CM = GetWorld()->GetPlayerCameraManager())
        {
            CM->SetViewCamera(CameraComp);
        }
    }
}

void ATempCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 입력 처리
    if (Controller)
    {
        UInputManager& Input = UInputManager::GetInstance();
        FVector MoveDirection(0.0f, 0.0f, 0.0f);

        // Get camera direction from player controller
        APlayerController* PC = Cast<APlayerController>(Controller);
        if (PC && PC->GetPlayerCameraManager())
        {
            APlayerCameraManager* CameraManager = PC->GetPlayerCameraManager();

            // Get camera forward and right vectors (flattened to ground plane)
            FVector Forward = CameraManager->GetActorForward();
            FVector Right = CameraManager->GetActorRight();
            Forward.Z = 0.0f;
            Right.Z = 0.0f;
            Forward.Normalize();
            Right.Normalize();

            // WASD input
            if (Input.IsKeyDown('W'))
            {
                MoveDirection += Forward;
            }
            if (Input.IsKeyDown('S'))
            {
                MoveDirection -= Forward;
            }
            if (Input.IsKeyDown('D'))
            {
                MoveDirection += Right;
            }
            if (Input.IsKeyDown('A'))
            {
                MoveDirection -= Right;
            }

            // Add movement input
            if (MoveDirection.SizeSquared() > 0.0f)
            {
                AddMovementInput(MoveDirection.GetNormalized(), 1.0f);
            }
        }

        // Apply movement (temporary implementation until CharacterMovementComponent is added)
        FVector InputVector = ConsumeMovementInputVector();
        if (InputVector.SizeSquared() > 0.0f)
        {
            FVector NewLocation = GetActorLocation() + InputVector * MovementSpeed * DeltaSeconds;
            SetActorLocation(NewLocation);
        }
    }
}
