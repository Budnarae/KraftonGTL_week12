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

    UFUNCTION(LuaBind)
    virtual bool CheckTransitionRule();

    UFUNCTION(LuaBind)
    void Evaluate();

    UFUNCTION(LuaBind)
    void SetRuleName(const FName& InName) { RuleName = InName; }

    UFUNCTION(LuaBind)
    FName GetRuleName() const { return RuleName; }

private:
    TDelegate<> TransitionDelegate;
    FName RuleName;
};