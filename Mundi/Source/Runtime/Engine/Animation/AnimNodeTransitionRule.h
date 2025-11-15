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

private:
    TDelegate<> TransitionDelegate;
};