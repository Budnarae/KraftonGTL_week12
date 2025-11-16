#pragma once

class UAnimNodeTransitionRule : public UObject
{
    DECLARE_CLASS(UAnimNodeTransitionRule, UObject)

public:
    enum class ComparisonOperator
    {
        GreaterThan,
        LessThan,
        GreaterThanOrEqualTo,
        LessThanOrEqualTo
    };

public:
    TDelegate<>& GetTransitionDelegate();

    virtual bool CheckTransitionRule();
    void Evaluate();

    void SetRuleName(const FName& InName) { RuleName = InName; }
    FName GetRuleName() const { return RuleName; }

private:
    TDelegate<> TransitionDelegate;
    FName RuleName;
};