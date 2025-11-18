#include "pch.h"
#include "AnimNotifyState.h"

void UAnimNotifyState::NotifyBegin()
{
    bEndAlreadyCalled = false;
}

void UAnimNotifyState::NotifyTick() {}

void UAnimNotifyState::NotifyEnd()
{
    bEndAlreadyCalled = true;
}

FName UAnimNotifyState::GetNotifyName()
{
    return Name;
}

void UAnimNotifyState::SetNotifyName(const FName& InName)
{
    Name = InName;
}

float UAnimNotifyState::GetStartTime()
{
    return StartTime;
}

void UAnimNotifyState::SetStartTime(float InStartTime)
{
    StartTime = InStartTime;
}

float UAnimNotifyState::GetDurationTime()
{
    return DurationTime;
}

void UAnimNotifyState::SetDurationTime(float InDurationTime)
{
    DurationTime = InDurationTime;
}

bool UAnimNotifyState::GetEndAlreadtCalled()
{
    return bEndAlreadyCalled;
}