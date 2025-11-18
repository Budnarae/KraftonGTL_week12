#include "pch.h"
#include "Character.h"
#include "World.h"
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
        MeshComponent->SetAnimInstance(InAnimInstance);
    }
}

UAnimInstance* ACharacter::GetAnimInstance() const
{
    if (MeshComponent)
    {
        return MeshComponent->GetAnimInstance();
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
