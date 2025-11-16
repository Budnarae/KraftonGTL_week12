#include "pch.h"
#include "AnimNodeTransitionRule.h"

IMPLEMENT_CLASS(UAnimNodeTransitionRule)

TDelegate<>& UAnimNodeTransitionRule::GetTransitionDelegate()
{
    return TransitionDelegate;
}

void UAnimNodeTransitionRule::Evaluate()
{
    if (CheckTransitionRule())
        TransitionDelegate.Broadcast();
}

bool UAnimNodeTransitionRule::CheckTransitionRule()
{
    return false;
}
