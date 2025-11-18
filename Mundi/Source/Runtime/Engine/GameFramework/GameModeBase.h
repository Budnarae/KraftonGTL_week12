#pragma once
#include "Actor.h"
#include "AGameModeBase.generated.h"

class APawn;
class APlayerController;

/**
 * AGameModeBase
 * 게임의 규칙과 플레이어 스폰을 담당하는 기본 게임 모드
 * 월드당 하나만 존재하며, 플레이어 초기화를 관리
 */
UCLASS(DisplayName="게임 모드", Description="게임 규칙과 플레이어 스폰을 관리")
class AGameModeBase : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    AGameModeBase();

protected:
    ~AGameModeBase() override;

public:
    // AActor interface
    virtual void BeginPlay() override;

    // GameModeBase interface

    /**
     * 게임 초기화 - 플레이어 컨트롤러 및 폰 생성
     */
    virtual void InitGame();

    /**
     * 기본 PlayerController 스폰
     */
    virtual APlayerController* SpawnDefaultPlayerController();

    /**
     * 기본 Pawn 스폰 및 빙의
     * @param Controller - Pawn을 제어할 PlayerController
     * @param SpawnTransform - 스폰 위치
     */
    virtual APawn* SpawnDefaultPawn(APlayerController* Controller, const FTransform& SpawnTransform);

    /**
     * 플레이어 스폰 위치 찾기
     * 기본 구현은 원점 반환, 하위 클래스에서 PlayerStart 액터 찾도록 오버라이드 가능
     */
    virtual FTransform FindPlayerStart();

    /**
     * 기본 Pawn 클래스 반환
     */
    UClass* GetDefaultPawnClass() const { return DefaultPawnClass; }

    /**
     * 기본 PlayerController 클래스 반환
     */
    UClass* GetPlayerControllerClass() const { return PlayerControllerClass; }

protected:
    /**
     * 스폰할 기본 Pawn 클래스
     * 에디터에서 설정 가능
     */
    UPROPERTY(EditAnywhere, Category="GameMode|Classes")
    UClass* DefaultPawnClass = nullptr;

    /**
     * 생성할 PlayerController 클래스
     * 에디터에서 설정 가능
     */
    UPROPERTY(EditAnywhere, Category="GameMode|Classes")
    UClass* PlayerControllerClass = nullptr;

    /**
     * 게임이 초기화되었는지 여부
     */
    bool bGameInitialized = false;
};
