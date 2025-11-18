#pragma once
#include "Actor.h"
#include "InputManager.h"
#include "APlayerController.generated.h"

class APawn;
class APlayerCameraManager;

/**
 * APlayerController
 * 플레이어의 입력을 받아 Pawn을 제어하는 액터
 * 입력 처리, 카메라 관리, HUD 등을 담당
 */
UCLASS(DisplayName="플레이어 컨트롤러", Description="플레이어 입력을 처리하고 폰을 제어")
class APlayerController : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    APlayerController();

protected:
    ~APlayerController() override;

public:
    // AActor interface
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay() override;

    // Controller interface

    /**
     * 지정된 Pawn을 빙의(제어)함
     * @param InPawn - 제어할 Pawn
     */
    virtual void Possess(APawn* InPawn);

    /**
     * 현재 제어 중인 Pawn의 빙의를 해제
     */
    virtual void UnPossess();

    /**
     * 현재 제어 중인 Pawn 반환
     */
    APawn* GetPawn() const { return ControlledPawn; }

    /**
     * 입력 처리 - 매 프레임 호출됨
     * 하위 클래스에서 오버라이드하여 입력 바인딩 구현
     */
    virtual void SetupInputComponent();

    /**
     * 플레이어 카메라 매니저 설정
     */
    void SetPlayerCameraManager(APlayerCameraManager* InCameraManager) { PlayerCameraManager = InCameraManager; }

    /**
     * 플레이어 카메라 매니저 반환
     */
    APlayerCameraManager* GetPlayerCameraManager() const { return PlayerCameraManager; }

    // Input helper functions (camera only)

    /**
     * 카메라 회전 (마우스)
     * @param DeltaYaw - Yaw 회전 변화량 (좌우)
     * @param DeltaPitch - Pitch 회전 변화량 (상하)
     */
    virtual void AddYawInput(float DeltaYaw);
    virtual void AddPitchInput(float DeltaPitch);

    /**
     * InputManager 참조 반환
     */
    UInputManager& GetInputManager() const { return UInputManager::GetInstance(); }

protected:
    // 현재 제어 중인 Pawn
    APawn* ControlledPawn = nullptr;

    // 플레이어 카메라 매니저
    APlayerCameraManager* PlayerCameraManager = nullptr;

    // 입력 감도
    UPROPERTY(EditAnywhere, Category="PlayerController|Input")
    float MouseSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, Category="PlayerController|Input")
    bool bInvertMouseY = false;

    // 입력이 활성화되어 있는지 여부
    UPROPERTY(EditAnywhere, Category="PlayerController|Input")
    bool bInputEnabled = true;
};
