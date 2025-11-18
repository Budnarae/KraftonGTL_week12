#pragma once

class UAnimationSequence;
class FArchive;

class UAnimNotify : public UObject
{
public:
    // **핵심 실행 함수**: 애니메이션 재생 중 Notify 지점에 도달했을 때 호출됩니다.
    // 자식 클래스에서 이 함수를 오버라이드하여 실제 기능을 구현합니다.
    virtual void Notify() = 0;

    // **바이너리 직렬화**: 기본 클래스 데이터(Name, TimeToNotify)를 처리합니다.
    // 자식 클래스는 Super::SerializeBinary(Ar) 호출 후 자신의 데이터를 처리합니다.
    // (JSON 직렬화는 UObject::Serialize 사용)
    virtual void SerializeBinary(FArchive& Ar)
    {
        if (Ar.IsSaving())
        {
            FString NotifyName = Name.ToString();
            float TimeToNotifyValue = TimeToNotify;

            Serialization::WriteString(Ar, NotifyName);
            Ar << TimeToNotifyValue;
        }
        else if (Ar.IsLoading())
        {
            FString NotifyName;
            float TimeToNotifyValue;

            Serialization::ReadString(Ar, NotifyName);
            Ar << TimeToNotifyValue;

            Name = FName(NotifyName);
            TimeToNotify = TimeToNotifyValue;
        }
    }

    FName GetNotifyName();
    void SetNotifyName(const FName& InName);

    float GetTimeToNotify();
    void SetTimeToNotify(float InTimeToNotify);
protected:
    FName Name{};
    float TimeToNotify{};
};