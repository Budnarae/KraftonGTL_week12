#pragma once

class UAnimNotifyState : public UObject
{
public:
    /** * Notify 구간의 시작 시점에 호출됩니다. 
     * 이 함수에서 상태를 활성화하거나 지속적인 효과를 생성합니다.
     */
    virtual void NotifyBegin();

    /** * Notify 구간 동안 매 프레임 호출됩니다.
     * 지속적인 업데이트 로직이나 효과를 구현할 때 사용됩니다.
     */
    virtual void NotifyTick() = 0;

    /** * Notify 구간의 종료 시점에 호출됩니다.
     * 이 함수에서 활성화된 상태나 효과를 비활성화하거나 정리합니다.
     */
    virtual void NotifyEnd() = 0;

    FName GetNotifyName();
    void SetNotifyName(const FName& InName);

    float GetStartTime();
    void SetStartTime(float InStartTime);

    float GetDurationTime();
    void SetDurationTime(float InDurationTime);

    bool GetEndAlreadtCalled();
protected:
    FName Name{};
    float StartTime{};
    float DurationTime{};
    bool bEndAlreadyCalled = false;
};