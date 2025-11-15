#include "pch.h"
#include "FloatComparisonRule.h"

IMPLEMENT_CLASS(UFloatComparisonRule)

void UFloatComparisonRule::SetComparisonOperator(const ComparisonOperator& InOperator)
{
    Operator = InOperator;
}

void UFloatComparisonRule::SetBaseValue(const float InValue)
{
    BaseValue = InValue;
}

void UFloatComparisonRule::SetComparisonValue(const float InValue)
{
    ComparisonValue = InValue;
}

bool UFloatComparisonRule::CheckTransitionRule()
{
    if (Operator == ComparisonOperator::GreaterThan)
    {
        if (ComparisonValue > BaseValue) return true;
    }
    else if (Operator == ComparisonOperator::LessThan)
    {
        if (ComparisonValue < BaseValue) return true;
    }
    else if (Operator == ComparisonOperator::GreaterThanOrEqualTo)
    {
        if (ComparisonValue >= BaseValue) return true;
    }
    else if (Operator == ComparisonOperator::LessThanOrEqualTo)
    {
        if (ComparisonValue <= BaseValue) return true;
    }
    return false;
}