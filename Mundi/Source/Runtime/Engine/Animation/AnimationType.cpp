#include "pch.h"
#include "AnimationType.h"
#include "Source/Runtime/Engine/Animation/AnimNodeTransitionRule.h"

// ====================================
// FAnimStateTransition 구현
// ====================================

void FAnimStateTransition::CleanupDelegate()
{
    if (AssociatedRule && DelegateHandle != 0)
    {
        AssociatedRule->GetTransitionDelegate().Remove(DelegateHandle);
        DelegateHandle = 0;
        AssociatedRule = nullptr;
    }
}

FAnimStateTransition::~FAnimStateTransition()
{
    CleanupDelegate();
}

FAnimStateTransition::FAnimStateTransition(const FAnimStateTransition& Other)
    : SourceState(Other.SourceState)
    , TargetState(Other.TargetState)
    , Index(Other.Index)
    , CanEnterTransition(false)  // 새로 생성되는 Transition은 항상 false로 초기화
    , AssociatedRule(nullptr)  // Delegate는 복사하지 않음
    , DelegateHandle(0)
    , BlendTime(Other.BlendTime)
    , BlendTimeElapsed(Other.BlendTimeElapsed)
    , BlendAlpha(Other.BlendAlpha)
    , bIsBlending(Other.bIsBlending)
{
    // Delegate는 복사 후 재바인딩 필요
}

FAnimStateTransition& FAnimStateTransition::operator=(const FAnimStateTransition& Other)
{
    if (this != &Other)
    {
        // 기존 Delegate 정리
        CleanupDelegate();

        SourceState = Other.SourceState;
        TargetState = Other.TargetState;
        Index = Other.Index;
        CanEnterTransition = Other.CanEnterTransition;
        BlendTime = Other.BlendTime;
        BlendTimeElapsed = Other.BlendTimeElapsed;
        BlendAlpha = Other.BlendAlpha;
        bIsBlending = Other.bIsBlending;

        // Delegate는 복사하지 않음 (재바인딩 필요)
        AssociatedRule = nullptr;
        DelegateHandle = 0;
    }
    return *this;
}