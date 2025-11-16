#pragma once
#include "AnimNodeTransitionRule.h"

class UFloatComparisonRule : public UAnimNodeTransitionRule
{
    DECLARE_CLASS(UFloatComparisonRule, UAnimNodeTransitionRule)

public:
    void SetComparisonOperator(const ComparisonOperator& InOperator);
    void SetBaseValue(const float InValue);
    void SetComparisonValue(const float InValue);

    bool CheckTransitionRule() override;

private:
    ComparisonOperator Operator;
    float ComparisonValue{};
    float BaseValue{};
};