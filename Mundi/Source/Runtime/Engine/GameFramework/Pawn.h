#pragma once
#include "Actor.h"
#include "APawn.generated.h"

class APlayerController;

/**
 * APawn
 * 플레이어나 AI가 제어할 수 있는 기본 액터
 * Controller에 의해 빙의(Possess)되어 제어됨
 */
UCLASS(DisplayName="폰", Description="제어 가능한 기본 액터")
class APawn : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    APawn();

protected:
    ~APawn() override;

public:
    // AActor interface
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay() override;

    // Pawn interface

    /**
     * 이 Pawn을 Controller가 빙의(제어)하도록 함
     * @param InController - 이 Pawn을 제어할 Controller
     */
    virtual void PossessedBy(APlayerController* InController);

    /**
     * Controller가 이 Pawn의 빙의를 해제함
     */
    virtual void UnPossessed();

    /**
     * 이 Pawn을 현재 제어 중인 Controller를 반환
     */
    APlayerController* GetController() const { return Controller; }

    /**
     * 이 Pawn이 플레이어에 의해 제어되고 있는지 확인
     */
    bool IsPlayerControlled() const { return Controller != nullptr; }

    /**
     * 입력 처리 - Controller로부터 호출됨
     * 하위 클래스에서 오버라이드하여 입력 로직 구현
     */
    virtual void SetupPlayerInputComponent() {}

    /**
     * 이동 입력
     * @param WorldDirection - 월드 공간에서의 이동 방향
     * @param ScaleValue - 이동 강도 (보통 -1.0 ~ 1.0)
     */
    virtual void AddMovementInput(const FVector& WorldDirection, float ScaleValue = 1.0f);

    /**
     * 현재 프레임의 누적된 이동 입력 벡터 반환
     */
    FVector GetPendingMovementInputVector() const { return ControlInputVector; }

    /**
     * 마지막으로 소비된 이동 입력 벡터 반환
     */
    FVector GetLastMovementInputVector() const { return LastControlInputVector; }

    /**
     * 이동 입력 벡터를 소비하고 초기화
     * 보통 MovementComponent에서 호출
     */
    virtual FVector ConsumeMovementInputVector();

protected:
    // 이 Pawn을 제어 중인 Controller
    APlayerController* Controller = nullptr;

    // 현재 프레임의 누적 이동 입력 벡터
    FVector ControlInputVector;

    // 이전 프레임에 소비된 이동 입력 벡터
    FVector LastControlInputVector;

    UPROPERTY(EditAnywhere, Category="Pawn")
    bool bCanBePossessed = true;
};
