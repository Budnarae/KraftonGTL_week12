#include "pch.h"
#include "GameModeBase.h"
#include "World.h"
#include "Pawn.h"
#include "Character.h"
#include "PlayerController.h"
#include "PlayerCameraManager.h"

AGameModeBase::AGameModeBase()
{
    ObjectName = "GameModeBase";
    bCanEverTick = false;

    // 기본 클래스 설정 (생성자에서 기본값 지정)
    DefaultPawnClass = ACharacter::StaticClass();
    PlayerControllerClass = APlayerController::StaticClass();
}

AGameModeBase::~AGameModeBase() = default;

void AGameModeBase::BeginPlay()
{
    Super::BeginPlay();

    // 게임 초기화 (플레이어 스폰)
    InitGame();
}

void AGameModeBase::InitGame()
{
    if (bGameInitialized)
    {
        return;
    }

    UE_LOG("[GameMode] Initializing game...");

    // 1. PlayerController 스폰
    APlayerController* PlayerController = SpawnDefaultPlayerController();
    if (!PlayerController)
    {
        UE_LOG("[GameMode] Failed to spawn PlayerController");
        return;
    }

    // 2. PlayerCameraManager 찾기 또는 생성
    APlayerCameraManager* CameraManager = GetWorld()->FindActor<APlayerCameraManager>();
    if (!CameraManager)
    {
        UE_LOG("[GameMode] No PlayerCameraManager found in world, creating one...");
        CameraManager = GetWorld()->SpawnActor<APlayerCameraManager>();
    }

    if (CameraManager)
    {
        PlayerController->SetPlayerCameraManager(CameraManager);
    }

    // 3. 기본 Pawn 스폰 및 빙의
    if (DefaultPawnClass)
    {
        FTransform SpawnTransform = FindPlayerStart();
        APawn* Pawn = SpawnDefaultPawn(PlayerController, SpawnTransform);

        if (Pawn)
        {
            UE_LOG("[GameMode] Player spawned successfully at (%f, %f, %f)",
                SpawnTransform.Translation.X,
                SpawnTransform.Translation.Y,
                SpawnTransform.Translation.Z);
        }
        else
        {
            UE_LOG("[GameMode] Failed to spawn default pawn");
        }
    }
    else
    {
        UE_LOG("[GameMode] No DefaultPawnClass set, skipping pawn spawn");
    }

    bGameInitialized = true;
}

APlayerController* AGameModeBase::SpawnDefaultPlayerController()
{
    UClass* ControllerClass = PlayerControllerClass;

    // PlayerControllerClass가 설정되지 않았으면 기본 APlayerController 사용
    if (!ControllerClass)
    {
        ControllerClass = APlayerController::StaticClass();
    }

    // PlayerController 생성
    APlayerController* NewController = Cast<APlayerController>(
        GetWorld()->SpawnActor(ControllerClass, FTransform())
    );

    if (NewController)
    {
        UE_LOG("[GameMode] PlayerController spawned: %s", ControllerClass->Name);
    }

    return NewController;
}

APawn* AGameModeBase::SpawnDefaultPawn(APlayerController* Controller, const FTransform& SpawnTransform)
{
    if (!Controller)
    {
        UE_LOG("[GameMode] Cannot spawn pawn: Controller is null");
        return nullptr;
    }

    if (!DefaultPawnClass)
    {
        UE_LOG("[GameMode] Cannot spawn pawn: DefaultPawnClass is null");
        return nullptr;
    }

    // Pawn 스폰
    APawn* NewPawn = Cast<APawn>(
        GetWorld()->SpawnActor(DefaultPawnClass, SpawnTransform)
    );

    if (NewPawn)
    {
        UE_LOG("[GameMode] Pawn spawned: %s", DefaultPawnClass->Name);

        // Controller가 Pawn 빙의
        Controller->Possess(NewPawn);

        UE_LOG("[GameMode] Controller possessed pawn");
    }

    return NewPawn;
}

FTransform AGameModeBase::FindPlayerStart()
{
    // TODO: PlayerStart 액터를 찾아서 위치 반환
    // 현재는 원점에서 약간 위에 스폰 (지면 충돌 방지)
    FTransform SpawnTransform;
    SpawnTransform.Translation = FVector(0.0f, 0.0f, 100.0f);
    return SpawnTransform;
}
