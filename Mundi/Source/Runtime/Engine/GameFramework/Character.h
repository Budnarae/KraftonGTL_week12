#pragma once
#include "Pawn.h"
#include "SkeletalMeshComponent.h"
#include "CapsuleComponent.h"
#include "ACharacter.generated.h"

/**
 * ACharacter
 * 스켈레탈 메시와 캡슐 충돌을 가진 인간형 캐릭터
 * APawn을 상속하여 플레이어나 AI가 제어 가능
 */
UCLASS(DisplayName="캐릭터", Description="스켈레탈 메시와 캡슐 충돌을 가진 캐릭터")
class ACharacter : public APawn
{
public:
    GENERATED_REFLECTION_BODY()

    ACharacter();

protected:
    ~ACharacter() override;

public:
    // AActor interface
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay() override;

    // APawn interface
    virtual void SetupPlayerInputComponent() override;

    // Character interface

    /**
     * 캡슐 컴포넌트 반환
     */
    UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }

    /**
     * 스켈레탈 메시 컴포넌트 반환
     */
    USkeletalMeshComponent* GetMesh() const { return MeshComponent; }

    /**
     * 스켈레탈 메시 설정
     */
    void SetSkeletalMesh(const FString& PathFileName);

    /**
     * 애니메이션 인스턴스 설정
     */
    void SetAnimInstance(UAnimInstance* InAnimInstance);

    /**
     * 애니메이션 인스턴스 반환
     */
    UAnimInstance* GetAnimInstance() const;

    // Copy/Serialize
    void DuplicateSubObjects() override;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
    // 충돌 및 루트 컴포넌트
    UCapsuleComponent* CapsuleComponent = nullptr;

    // 스켈레탈 메시
    USkeletalMeshComponent* MeshComponent = nullptr;

    // 이동 속도
    UPROPERTY(EditAnywhere, Category="Character|Movement")
    float MovementSpeed = 300.0f;
};
