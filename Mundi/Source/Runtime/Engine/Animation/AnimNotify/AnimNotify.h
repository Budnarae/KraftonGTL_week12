#pragma once

class UAnimationSequence;

class UAnimNotify : public UObject
{
public:
    // **핵심 실행 함수**: 애니메이션 재생 중 Notify 지점에 도달했을 때 호출됩니다.
    // 자식 클래스에서 이 함수를 오버라이드하여 실제 기능을 구현합니다.
    virtual void Notify() = 0;

    FName GetNotifyName();
    void SetNotifyName(const FName& InName);

    float GetTimeToNotify();
    void SetTimeToNotify(float InTimeToNotify);
protected:
    FName Name{};
    float TimeToNotify{};
};