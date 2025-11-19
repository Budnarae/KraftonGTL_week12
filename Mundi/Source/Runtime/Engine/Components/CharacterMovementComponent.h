#pragma once
#include "MovementComponent.h"
#include "UCharacterMovementComponent.generated.h"

class ACharacter;

/**
 * EMovementMode
 * 캐릭터의 현재 이동 모드
 */
enum class EMovementMode : uint8
{
	None,
	Walking,
	Falling,
	Flying,
	Swimming
};

/**
 * UCharacterMovementComponent
 * 캐릭터의 걷기, 점프, 낙하 등 이동을 처리하는 컴포넌트
 */
UCLASS(DisplayName="캐릭터 무브먼트", Description="캐릭터의 이동을 처리하는 컴포넌트")
class UCharacterMovementComponent : public UMovementComponent
{
public:
	GENERATED_REFLECTION_BODY()

	UCharacterMovementComponent();

protected:
	~UCharacterMovementComponent() override;

public:
	// Life Cycle
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaSeconds) override;

	// Movement Actions
	virtual void Jump();
	virtual void StopJumping();
	virtual bool CanJump() const;
	virtual bool IsJumping() const { return bIsJumping; }

	// Movement State
	bool IsMovingOnGround() const { return MovementMode == EMovementMode::Walking; }
	bool IsFalling() const { return MovementMode == EMovementMode::Falling; }
	bool IsFlying() const { return MovementMode == EMovementMode::Flying; }

	EMovementMode GetMovementMode() const { return MovementMode; }
	void SetMovementMode(EMovementMode NewMovementMode);

	// Getters for movement properties
	float GetMaxWalkSpeed() const { return MaxWalkSpeed; }
	void SetMaxWalkSpeed(float NewMaxWalkSpeed) { MaxWalkSpeed = NewMaxWalkSpeed; }

	float GetMaxAcceleration() const { return MaxAcceleration; }
	void SetMaxAcceleration(float NewAcceleration) { MaxAcceleration = NewAcceleration; }

	float GetGravityZ() const { return GravityZ; }
	void SetGravityZ(float NewGravity) { GravityZ = NewGravity; }

	float GetJumpZVelocity() const { return JumpZVelocity; }
	void SetJumpZVelocity(float NewJumpVelocity) { JumpZVelocity = NewJumpVelocity; }

	// Character reference
	ACharacter* GetCharacterOwner() const { return CharacterOwner; }

protected:
	// Movement calculations
	virtual void PerformMovement(float DeltaSeconds);
	virtual void ApplyInputToVelocity(float DeltaSeconds);
	virtual void ApplyFriction(float DeltaSeconds, float Friction);
	virtual void ApplyGravity(float DeltaSeconds);
	virtual void MoveUpdatedComponent(const FVector& Delta, float DeltaSeconds);

	// Ground checking
	virtual void CheckForGround();
	virtual bool IsOnGround() const;

	// Helper functions
	FVector GetInputVector() const;
	float GetMaxSpeed() const;

protected:
	// Character owner reference
	ACharacter* CharacterOwner = nullptr;

	// Movement mode
	EMovementMode MovementMode = EMovementMode::Walking;

	// Walking (단위: 미터)
	UPROPERTY(EditAnywhere, Category="Character Movement: Walking")
	float MaxWalkSpeed = 6.0f;  // 6 m/s

	UPROPERTY(EditAnywhere, Category="Character Movement: Walking")
	float MaxAcceleration = 20.0f;  // 20 m/s²

	UPROPERTY(EditAnywhere, Category="Character Movement: Walking")
	float GroundFriction = 8.0f;

	UPROPERTY(EditAnywhere, Category="Character Movement: Walking")
	float BrakingDecelerationWalking = 20.0f;  // 20 m/s²

	// Jumping/Falling (단위: 미터)
	UPROPERTY(EditAnywhere, Category="Character Movement: Jumping/Falling")
	float JumpZVelocity = 4.2f;  // 4.2 m/s

	UPROPERTY(EditAnywhere, Category="Character Movement: Jumping/Falling")
	float GravityZ = -9.8f;  // 9.8 m/s² (지구 중력)

	UPROPERTY(EditAnywhere, Category="Character Movement: Jumping/Falling")
	float AirControl = 0.2f;

	UPROPERTY(EditAnywhere, Category="Character Movement: Jumping/Falling")
	float FallingLateralFriction = 0.0f;

	// Jump state
	bool bIsJumping = false;
	bool bWantsToJump = false;
	int32 JumpCurrentCount = 0;

	UPROPERTY(EditAnywhere, Category="Character Movement: Jumping/Falling")
	int32 JumpMaxCount = 1;

	// Ground detection
	float GroundCheckDistance = 0.02f;  // 2cm
	bool bOnGround = true;
};
