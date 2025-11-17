#pragma once
#include "AnimNodeTransitionRule.h"
#include "AnimNode.h"

class USkeletalMeshComponent;
class UAnimationSequence;
class UAnimInstance : public UObject
{
public:
    DECLARE_CLASS(UAnimInstance, UObject)
    
    template<typename TNode, typename... TArgs> 
    TNode* NewNode(TArgs&&... Args) 
    { 
        static_assert(std::is_base_of_v<FAnimNode_Base, TNode>, "TNode must derive from FAnimNode_Base"); 
        TNode* Node = new TNode(std::forward<TArgs>(Args)...); 
        NodePool.Add(Node); 
        return Node; 
    }

    virtual ~UAnimInstance();

protected:
    TArray<FAnimNode_Base*> NodePool;

public:
    void SetSkeletalComponent(USkeletalMeshComponent* InSkeletalMeshComponent);
    USkeletalMeshComponent* GetSkeletalComponent() const { return OwnerSkeletalComp; }

    FAnimNode_StateMachine ASM;
    FAnimNode_Base* RootNode = nullptr;

    // 현재 포즈를 저장할 변수
    FPoseContext CurrentPose;

    float CurTime = 0.0;

    /**
     * @brief AnimInstance 초기화 (최초 1회 호출)
     */
    void Initialize();
    
    /**
     * @brief 매 프레임 애니메이션 시간을 업데이트하고 본 포즈를 갱신합니다
     * @param DeltaTime 프레임 델타 타임
     */
    void UpdateAnimation(float DeltaTime);

protected:
    /* Unreal Style */

    // - 게임 로직 기준 Update (Tick) 단계
    // - 파라미터 업데이트, Transition 조건에 필요한 변수 갱신
    virtual void NativeUpdateAnimation(float DeltaSeconds);

    // - AnimGraph Evaluate 단계 수행
    // - 최종 Pose 계산
    virtual void EvaluateAnimation();

public:
    FPoseContext& GetCurrentPose() { return CurrentPose; };

// ====================================
// Transition Rule Management
// ====================================
public:
    /**
     * @brief Transition Rule 추가
     * @param InRule 추가할 Rule 객체
     */
    void AddTransitionRule(UAnimNodeTransitionRule* InRule);

    /**
     * @brief 이름으로 Transition Rule 제거
     * @param RuleName 제거할 Rule 이름
     */
    void RemoveTransitionRule(const FName& RuleName);

    /**
     * @brief 이름으로 Transition Rule 찾기
     * @param RuleName 찾을 Rule 이름
     * @return Rule 객체, 없으면 nullptr
     */
    UAnimNodeTransitionRule* FindTransitionRuleByName(const FName& RuleName) const;

    /**
     * @brief 모든 Transition Rule 반환
     */
    const TArray<UAnimNodeTransitionRule*>& GetAllTransitionRules() const { return TransitionRules; }

// ====================================
// Animation State Machine Setup
// ====================================
public:
    /**
     * @brief Animation State Machine 초기화 (테스트용)
     */
    void InitializeAnimationStateMachine();

protected:
    USkeletalMeshComponent* OwnerSkeletalComp = nullptr;

    // Transition Rule 관리
    TArray<UAnimNodeTransitionRule*> TransitionRules;

    // 10초마다 애니메이션 전환 타이머
    float TransitionTimer = 0.0f;
    const float TransitionInterval = 10.0f;
    FAnimState* PreviousState = nullptr;  // 상태 변경 감지용

private:
    bool bIsInitialized = false;
};