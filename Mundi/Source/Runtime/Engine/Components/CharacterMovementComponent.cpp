#include "pch.h"
#include "CharacterMovementComponent.h"
#include "Character.h"
#include "SceneComponent.h"

IMPLEMENT_CLASS(UCharacterMovementComponent)

UCharacterMovementComponent::UCharacterMovementComponent()
{
	ObjectName = "CharacterMovementComponent";
	bCanEverTick = true;
}

UCharacterMovementComponent::~UCharacterMovementComponent()
{
}

void UCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Get character owner
	if (Owner)
	{
		CharacterOwner = Cast<ACharacter>(Owner);
	}

	// Set updated component to character's root
	if (CharacterOwner && !UpdatedComponent)
	{
		SetUpdatedComponent(CharacterOwner->GetRootComponent());
	}

	// Initialize movement mode
	SetMovementMode(EMovementMode::Walking);
}

void UCharacterMovementComponent::TickComponent(float DeltaSeconds)
{
	Super::TickComponent(DeltaSeconds);

	if (!CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	PerformMovement(DeltaSeconds);
}

void UCharacterMovementComponent::PerformMovement(float DeltaSeconds)
{
	// Check ground state
	CheckForGround();

	// Apply input to velocity
	ApplyInputToVelocity(DeltaSeconds);

	// Handle jumping
	if (bWantsToJump && CanJump())
	{
		Velocity.Z = JumpZVelocity;
		bIsJumping = true;
		bWantsToJump = false;
		JumpCurrentCount++;
		SetMovementMode(EMovementMode::Falling);
	}

	// Apply gravity if falling
	if (IsFalling())
	{
		ApplyGravity(DeltaSeconds);
	}
	else
	{
		// Apply ground friction
		ApplyFriction(DeltaSeconds, GroundFriction);
	}

	// Calculate delta movement
	FVector Delta = Velocity * DeltaSeconds;

	// Move the character
	MoveUpdatedComponent(Delta, DeltaSeconds);

	// Update movement mode based on ground state
	if (bOnGround && Velocity.Z <= 0.0f)
	{
		if (IsFalling())
		{
			SetMovementMode(EMovementMode::Walking);
			Velocity.Z = 0.0f;
			bIsJumping = false;
			JumpCurrentCount = 0;
		}
	}
	else if (!bOnGround && IsMovingOnGround())
	{
		SetMovementMode(EMovementMode::Falling);
	}
}

void UCharacterMovementComponent::ApplyInputToVelocity(float DeltaSeconds)
{
	FVector InputVector = GetInputVector();

	if (InputVector.SizeSquared() > 0.0f)
	{
		// Normalize and scale by max acceleration
		InputVector = InputVector.GetNormalized();
		FVector DesiredAcceleration = InputVector * MaxAcceleration;

		// Apply air control reduction if falling
		if (IsFalling())
		{
			DesiredAcceleration *= AirControl;
		}

		// Add to velocity
		Velocity.X += DesiredAcceleration.X * DeltaSeconds;
		Velocity.Y += DesiredAcceleration.Y * DeltaSeconds;

		// Clamp horizontal velocity to max speed
		float MaxSpeed = GetMaxSpeed();
		FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
		if (HorizontalVelocity.Size() > MaxSpeed)
		{
			HorizontalVelocity = HorizontalVelocity.GetNormalized() * MaxSpeed;
			Velocity.X = HorizontalVelocity.X;
			Velocity.Y = HorizontalVelocity.Y;
		}
	}
	else if (IsMovingOnGround())
	{
		// Apply braking deceleration when no input
		float BrakingDecel = BrakingDecelerationWalking * DeltaSeconds;
		FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
		float CurrentSpeed = HorizontalVelocity.Size();

		if (CurrentSpeed > 0.0f)
		{
			float NewSpeed = FMath::Max(0.0f, CurrentSpeed - BrakingDecel);
			float SpeedRatio = NewSpeed / CurrentSpeed;
			Velocity.X *= SpeedRatio;
			Velocity.Y *= SpeedRatio;
		}
	}
}

void UCharacterMovementComponent::ApplyFriction(float DeltaSeconds, float Friction)
{
	if (Friction <= 0.0f)
	{
		return;
	}

	FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	float CurrentSpeed = HorizontalVelocity.Size();

	if (CurrentSpeed > 0.0f)
	{
		float FrictionForce = Friction * DeltaSeconds;
		float NewSpeed = FMath::Max(0.0f, CurrentSpeed - FrictionForce * CurrentSpeed);
		float SpeedRatio = NewSpeed / CurrentSpeed;
		Velocity.X *= SpeedRatio;
		Velocity.Y *= SpeedRatio;
	}
}

void UCharacterMovementComponent::ApplyGravity(float DeltaSeconds)
{
	Velocity.Z += GravityZ * DeltaSeconds;

	// Apply falling lateral friction
	if (FallingLateralFriction > 0.0f)
	{
		ApplyFriction(DeltaSeconds, FallingLateralFriction);
	}
}

void UCharacterMovementComponent::MoveUpdatedComponent(const FVector& Delta, float DeltaSeconds)
{
	if (!UpdatedComponent)
	{
		return;
	}

	FVector CurrentLocation = UpdatedComponent->GetWorldLocation();
	FVector NewLocation = CurrentLocation + Delta;

	// Simple ground clamping (temporary - should use proper collision)
	if (NewLocation.Z < 0.0f && Velocity.Z <= 0.0f)
	{
		NewLocation.Z = 0.0f;
		Velocity.Z = 0.0f;
		bOnGround = true;
	}

	UpdatedComponent->SetWorldLocation(NewLocation);
}

void UCharacterMovementComponent::CheckForGround()
{
	if (!UpdatedComponent)
	{
		bOnGround = false;
		return;
	}

	FVector CurrentLocation = UpdatedComponent->GetWorldLocation();

	// Simple ground check (temporary - should use raycasting)
	// Consider on ground if Z position is at or below ground level
	bOnGround = (CurrentLocation.Z <= GroundCheckDistance);
}

bool UCharacterMovementComponent::IsOnGround() const
{
	return bOnGround;
}

FVector UCharacterMovementComponent::GetInputVector() const
{
	if (CharacterOwner)
	{
		return CharacterOwner->ConsumeMovementInputVector();
	}
	return FVector::Zero();
}

float UCharacterMovementComponent::GetMaxSpeed() const
{
	switch (MovementMode)
	{
	case EMovementMode::Walking:
		return MaxWalkSpeed;
	case EMovementMode::Falling:
		return MaxWalkSpeed;
	case EMovementMode::Flying:
		return MaxWalkSpeed;
	default:
		return 0.0f;
	}
}

void UCharacterMovementComponent::Jump()
{
	bWantsToJump = true;
}

void UCharacterMovementComponent::StopJumping()
{
	bWantsToJump = false;
}

bool UCharacterMovementComponent::CanJump() const
{
	// Can jump if on ground or have remaining jumps
	return bOnGround || (JumpCurrentCount < JumpMaxCount);
}

void UCharacterMovementComponent::SetMovementMode(EMovementMode NewMovementMode)
{
	if (MovementMode != NewMovementMode)
	{
		MovementMode = NewMovementMode;
	}
}
