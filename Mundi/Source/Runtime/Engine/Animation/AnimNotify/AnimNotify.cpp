#include "pch.h"
#include "AnimNotify.h"

FName UAnimNotify::GetNotifyName()
{
    return Name;
}

void UAnimNotify::SetNotifyName(const FName& InName)
{
    Name = InName;
}

float UAnimNotify::GetTimeToNotify()
{
    return TimeToNotify;
}

void UAnimNotify::SetTimeToNotify(float InTimeToNotify)
{
    TimeToNotify = InTimeToNotify;
}