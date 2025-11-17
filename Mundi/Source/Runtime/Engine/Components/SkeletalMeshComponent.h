#pragma once
#include "SkinnedMeshComponent.h"
#include "USkeletalMeshComponent.generated.h"

enum class EAnimationMode : uint8
{
    AnimationSingleNode,  ///< AnimInstance 없이 단일 애니메이션 재생
    AnimationBlueprint    ///< AnimInstance를 통한 복잡한 애니메이션 로직 (블렌딩, 스테이트 머신)
};

class UWorld;
class UAnimInstance;

UCLASS(DisplayName="스켈레탈 메시 컴포넌트", Description="스켈레탈 메시를 렌더링하는 컴포넌트입니다")
class USkeletalMeshComponent : public USkinnedMeshComponent
{
public:
    GENERATED_REFLECTION_BODY()
    
    USkeletalMeshComponent();
    ~USkeletalMeshComponent();

    // FOR TEST
    UPROPERTY(EditAnywhere, Category = "Animation", Range = "0, 1")
    float Alpha = 0.0;

    void OnRegister(UWorld* InWorld) override; // AnimInstance 초기화
    void TickComponent(float DeltaTime) override;
    void SetSkeletalMesh(const FString& PathFileName) override;

// ====================================
// Animation System
// ====================================
public:
    UPROPERTY(EditAnywhere, Category = "Animation")
    EAnimationMode AnimationMode = EAnimationMode::AnimationSingleNode;

    void SetAnimInstance(UAnimInstance* NewAnimInstanceClass);
    UAnimInstance* GetAnimInstance() const {return AnimInstance;};

    void SetAnimationMode(EAnimationMode InAnimationMode) { AnimationMode = InAnimationMode; }

    /**
     * @brief 애니메이션 모드를 설정합니다 (Lua용 - int 버전)
     * @param InAnimationMode 0=SingleNode, 1=AnimationBlueprint
     */
    UFUNCTION(LuaBind, DisplayName="SetAnimationModeInt")
    void SetAnimationModeInt(int InAnimationMode) { AnimationMode = static_cast<EAnimationMode>(InAnimationMode); }

    /**
     * @brief AnimInstance를 초기화/재생성합니다 (Lua용)
     */
    UFUNCTION(LuaBind)
    void InitAnimInstance();

// ====================================
// Pose Management
// ====================================
public:
    /**
     * @brief CurrentLocalSpacePose의 변경사항을 ComponentSpace -> FinalMatrices 계산까지 모두 수행
     */
    void ForceRecomputePose();
    
    /**
     * @brief AnimInstance에서 애니메이션 Update용 Getter 
     */
    TArray<FTransform>& GetLocalSpacePose() { return CurrentLocalSpacePose; }

// ====================================
// Bone Transform Manipulation (Editor/Runtime)
// ====================================
public:
    /**
     * @brief 특정 뼈의 부모 기준 로컬 트랜스폼을 설정
     * @param BoneIndex 수정할 뼈의 인덱스
     * @param NewLocalTransform 새로운 부모 기준 로컬 FTransform
     */
    void SetBoneLocalTransform(int32 BoneIndex, const FTransform& NewLocalTransform);

    void SetBoneWorldTransform(int32 BoneIndex, const FTransform& NewWorldTransform);
    
    /**
     * @brief 특정 뼈의 현재 로컬 트랜스폼을 반환
     */
    FTransform GetBoneLocalTransform(int32 BoneIndex) const;
    
    /**
     * @brief 기즈모를 렌더링하기 위해 특정 뼈의 월드 트랜스폼을 계산
     */
    FTransform GetBoneWorldTransform(int32 BoneIndex);

protected:
    /**
     * @brief CurrentLocalSpacePose를 기반으로 CurrentComponentSpacePose 채우기
     */
    void UpdateComponentSpaceTransforms();

    /**
     * @brief CurrentComponentSpacePose를 기반으로 TempFinalSkinningMatrices 채우기
     */
    void UpdateFinalSkinningMatrices();

    
// ====================================
// Animation Data
// ====================================
protected:
    /** 
     * @brief 애니메이션 로직 담당
     */
    UAnimInstance* AnimInstance = nullptr;

// ====================================
// Pose Data (Bone Transforms)
// ====================================
protected:
    /**
     * @brief 각 뼈의 부모 기준 로컬 트랜스폼
     */
    TArray<FTransform> CurrentLocalSpacePose;

    /**
     * @brief LocalSpacePose로부터 계산된 컴포넌트 기준 트랜스폼
     */
    TArray<FTransform> CurrentComponentSpacePose;

    /**
     * @brief 부모에게 보낼 최종 스키닝 행렬 (임시 계산용)
     */
    TArray<FMatrix> TempFinalSkinningMatrices;
    /**
     * @brief CPU 스키닝에 전달할 최종 노말 스키닝 행렬
     */
    TArray<FMatrix> TempFinalSkinningNormalMatrices;
};


