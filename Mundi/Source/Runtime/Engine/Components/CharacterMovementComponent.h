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
	Walking,
	Falling
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
	UFUNCTION(LuaBind, DisplayName="Jump")
	virtual void Jump();

	UFUNCTION(LuaBind, DisplayName="StopJumping")
	virtual void StopJumping();

	UFUNCTION(LuaBind, DisplayName="CanJump")
	virtual bool CanJump() const;

	UFUNCTION(LuaBind, DisplayName="IsJumping")
	bool IsJumping() const { return bIsJumping; }

	// Movement State
	UFUNCTION(LuaBind, DisplayName="IsMovingOnGround")
	bool IsMovingOnGround() const { return MovementMode == EMovementMode::Walking; }

	UFUNCTION(LuaBind, DisplayName="IsFalling")
	bool IsFalling() const { return MovementMode == EMovementMode::Falling; }

	EMovementMode GetMovementMode() const { return MovementMode; }
	void SetMovementMode(EMovementMode NewMovementMode);

	// Getters for movement properties
	UFUNCTION(LuaBind, DisplayName="GetVelocity")
	FVector GetVelocity() const { return Velocity; }

	UFUNCTION(LuaBind, DisplayName="GetMaxWalkSpeed")
	float GetMaxWalkSpeed() const { return MaxWalkSpeed; }

	UFUNCTION(LuaBind, DisplayName="SetMaxWalkSpeed")
	void SetMaxWalkSpeed(float NewMaxWalkSpeed) { MaxWalkSpeed = NewMaxWalkSpeed; }

	UFUNCTION(LuaBind, DisplayName="GetMaxAcceleration")
	float GetMaxAcceleration() const { return MaxAcceleration; }

	UFUNCTION(LuaBind, DisplayName="SetMaxAcceleration")
	void SetMaxAcceleration(float NewAcceleration) { MaxAcceleration = NewAcceleration; }

	UFUNCTION(LuaBind, DisplayName="GetGravityZ")
	float GetGravityZ() const { return GravityZ; }

	UFUNCTION(LuaBind, DisplayName="SetGravityZ")
	void SetGravityZ(float NewGravity) { GravityZ = NewGravity; }

	UFUNCTION(LuaBind, DisplayName="GetJumpZVelocity")
	float GetJumpZVelocity() const { return JumpZVelocity; }

	UFUNCTION(LuaBind, DisplayName="SetJumpZVelocity")
	void SetJumpZVelocity(float NewJumpVelocity) { JumpZVelocity = NewJumpVelocity; }

	// Character reference
	ACharacter* GetCharacterOwner() const { return CharacterOwner; }

protected:
	// Movement calculations
	virtual void PerformMovement(float DeltaSeconds);
	virtual void ApplyInputToVelocity(float DeltaSeconds);
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
	float BrakingDecelerationWalking = 20.0f;  // 20 m/s²

	// Jumping/Falling (단위: 미터)
	UPROPERTY(EditAnywhere, Category="Character Movement: Jumping/Falling")
	float JumpZVelocity = 4.2f;  // 4.2 m/s

	UPROPERTY(EditAnywhere, Category="Character Movement: Jumping/Falling")
	float GravityZ = -9.8f;  // 9.8 m/s² (지구 중력)

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
