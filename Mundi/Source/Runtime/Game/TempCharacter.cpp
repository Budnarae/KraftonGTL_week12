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
    SpringArm->TargetArmLength = 5.f;
    SpringArm->SetRelativeLocation(FVector(0, 0, 2.f));  // 캐릭터 머리 높이

    // 카메라 컴포넌트 생성 (스프링 암에 부착)
    CameraComp = CreateDefaultSubobject<UCameraComponent>("DefaultCamera");
    CameraComp->SetupAttachment(SpringArm, EAttachmentRule::KeepRelative);
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

    UInputManager& Input = UInputManager::GetInstance();

    // 마우스 입력으로 SpringArm 회전
    if (SpringArm)
    {
        FVector2D MouseDelta = Input.GetMouseDelta();

        CameraYaw += MouseDelta.X * MouseSensitivity;
        CameraPitch += MouseDelta.Y * MouseSensitivity;  // Y축 반전

        // Pitch 제한 (-89 ~ 89도)
        CameraPitch = FMath::Clamp(CameraPitch, -89.0f, 89.0f);

        // SpringArm 회전 적용
        FQuat NewRotation = FQuat::MakeFromEulerZYX(FVector(0.f, CameraPitch, CameraYaw));
        SpringArm->SetRelativeRotation(NewRotation);
    }

    // 입력 처리
    if (Controller)
    {
        FVector MoveDirection(0.0f, 0.0f, 0.0f);

        // 카메라 방향 기반 이동
        if (CameraComp)
        {
            // Get camera forward and right vectors (flattened to ground plane)
            FVector Forward = CameraComp->GetForward();
            FVector Right = CameraComp->GetRight();
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
            // 이동 방향으로 캐릭터 회전
            float TargetYaw = atan2f(InputVector.Y, InputVector.X) * (180.0f / 3.14159265f);
            FQuat TargetRotation = FQuat::MakeFromEulerZYX(FVector(0.0f, 0.0f, TargetYaw));
            FQuat CurrentRotation = GetMesh()->GetRelativeRotation();
            float InterpSpeed = FMath::Clamp(DeltaSeconds * RotationSpeed / 36.0f, 0.0f, 1.0f);
            FQuat NewRotation = FQuat::Slerp(CurrentRotation, TargetRotation, InterpSpeed);
            GetMesh()->SetRelativeRotation(NewRotation);

            // 이동 적용
            FVector NewLocation = GetActorLocation() + InputVector * MovementSpeed * DeltaSeconds;
            SetActorLocation(NewLocation);
        }
    }
}
