#include "pch.h"
#include "TempCharacter.h"
#include "World.h"
#include "PlayerController.h"
#include "PlayerCameraManager.h"
#include "CameraComponent.h"
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

    // 스프링 암 생성
    SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
    SpringArm->SetupAttachment(RootComponent, EAttachmentRule::KeepRelative);
    SpringArm->TargetArmLength = 300.0f;
    SpringArm->SocketOffset = FVector(0, 0, 50);  // 약간 위로 오프셋
    SpringArm->SetRelativeLocation(FVector(0, 0, 80));  // 캐릭터 머리 높이

    // 카메라 컴포넌트 생성 (스프링 암에 부착)
    CameraComp = CreateDefaultSubobject<UCameraComponent>("DefaultCamera");
    CameraComp->SetupAttachment(SpringArm, EAttachmentRule::KeepRelative);
    CameraComp->SetFarClipPlane(10000.0f);  // 원거리 클리핑 확장
}

void ATempCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 내 카메라를 뷰 카메라로 설정
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
        if (PC && PC->GetPlayerCameraManager() && PC->GetPlayerCameraManager()->GetViewCamera())
        {
            UCameraComponent* Camera = PC->GetPlayerCameraManager()->GetViewCamera();

            // Get camera forward and right vectors (flattened to ground plane)
            FVector Forward = Camera->GetForward();
            FVector Right = Camera->GetRight();
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
