#include "pch.h"
#include "AnimationType.h"

// ====================================
// FAnimStateTransition 구현
// ====================================

FAnimStateTransition::~FAnimStateTransition()
{
    // TransitionConditionFunc는 자동으로 해제됨 (sol::function 소멸자)
}

FAnimStateTransition::FAnimStateTransition(const FAnimStateTransition& Other)
    : SourceState(Other.SourceState)
    , TargetState(Other.TargetState)
    , Index(Other.Index)
    , CanEnterTransition(false)  // 새로 생성되는 Transition은 항상 false로 초기화
    , TransitionConditionFunc(Other.TransitionConditionFunc)  // Lua 함수 복사
    , BlendTime(Other.BlendTime)
    , BlendTimeElapsed(Other.BlendTimeElapsed)
    , BlendAlpha(Other.BlendAlpha)
    , bIsBlending(Other.bIsBlending)
{
}

FAnimStateTransition& FAnimStateTransition::operator=(const FAnimStateTransition& Other)
{
    if (this != &Other)
    {
        SourceState = Other.SourceState;
        TargetState = Other.TargetState;
        Index = Other.Index;
        CanEnterTransition = Other.CanEnterTransition;
        TransitionConditionFunc = Other.TransitionConditionFunc;
        BlendTime = Other.BlendTime;
        BlendTimeElapsed = Other.BlendTimeElapsed;
        BlendAlpha = Other.BlendAlpha;
        bIsBlending = Other.bIsBlending;
    }
    return *this;
}