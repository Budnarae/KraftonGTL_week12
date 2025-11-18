#include "pch.h"
#include "Character.h"
#include "World.h"
#include "PlayerController.h"
#include "PlayerCameraManager.h"
#include "InputManager.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"

ACharacter::ACharacter()
{
    ObjectName = "Character";
    bCanEverTick = true;

    // Create capsule component for collision
    CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
    SetRootComponent(CapsuleComponent);

    // Create skeletal mesh component
    MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("MeshComponent");
    MeshComponent->SetupAttachment(CapsuleComponent);

    // Offset mesh so feet are at capsule bottom
    // Usually mesh origin is at feet, so offset up by half height (default 100.0f)
    MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -100.0f));
}

ACharacter::~ACharacter() = default;

void ACharacter::BeginPlay()
{
    Super::BeginPlay();
}

void ACharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Temp Input Sys
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

void ACharacter::EndPlay()
{
    Super::EndPlay();
}

void ACharacter::SetupPlayerInputComponent()
{
    Super::SetupPlayerInputComponent();

    // Input processing is done in Tick() using UInputManager
    // This function exists to override the parent's version
}

void ACharacter::SetSkeletalMesh(const FString& PathFileName)
{
    if (MeshComponent)
    {
        MeshComponent->SetSkeletalMesh(PathFileName);
    }
}

void ACharacter::SetAnimInstance(UAnimInstance* InAnimInstance)
{
    if (MeshComponent)
    {
        MeshComponent->SetAnimInstanceClass(InAnimInstance);
    }
}

UAnimInstance* ACharacter::GetAnimInstance() const
{
    if (MeshComponent)
    {
        return MeshComponent->GetAnimInstanceClass();
    }
    return nullptr;
}

void ACharacter::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    // Find components after duplication
    for (UActorComponent* Component : OwnedComponents)
    {
        if (auto* Comp = Cast<UCapsuleComponent>(Component))
        {
            CapsuleComponent = Comp;
        }
        else if (auto* Comp = Cast<USkeletalMeshComponent>(Component))
        {
            MeshComponent = Comp;
        }
    }
}

void ACharacter::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // Restore component references after loading
        CapsuleComponent = Cast<UCapsuleComponent>(RootComponent);

        for (UActorComponent* Component : OwnedComponents)
        {
            if (auto* Comp = Cast<USkeletalMeshComponent>(Component))
            {
                MeshComponent = Comp;
                break;
            }
        }
    }
}
