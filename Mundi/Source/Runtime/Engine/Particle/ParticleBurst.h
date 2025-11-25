#pragma once
#include "UParticleBurst.generated.h"
// ============================================================================
// FParticleBurst - 버스트 스폰 정보
// ============================================================================
// 특정 시간에 한 번에 많은 파티클을 생성하는 "버스트"를 정의합니다.
//
// 사용 예시:
// - 폭발 효과: 0초에 100개 파티클을 한 번에 생성
// - 불꽃놀이: 여러 시점에 다양한 양의 파티클 생성
// ============================================================================
UCLASS(DisplayName = "파티클 버스트", Description = "파티클 생성 기능.")
class UParticleBurst : public UObject
{
public:
    GENERATED_REFLECTION_BODY()
    // ------------------------------------------------------------------------
    // Count (버스트 개수)
    // ------------------------------------------------------------------------
    // 버스트 시 생성할 파티클 개수입니다.
    // CountLow가 -1이 아니면, CountLow~Count 범위에서 랜덤 선택됩니다.
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "[버스트]", Tooltip = "버스트 갯수")
    int32 Count = 10;

    // ------------------------------------------------------------------------
    // CountLow (랜덤 범위 최소값)
    // ------------------------------------------------------------------------
    // -1: Count 값을 고정으로 사용
    // 0 이상: CountLow~Count 범위에서 랜덤 선택
    //
    // 예시:
    // - CountLow=-1, Count=10 → 항상 10개
    // - CountLow=5, Count=15 → 5~15개 랜덤
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "[버스트]", Tooltip = "버스트 최소 갯수")
    int32 CountLow = -1;

    // ------------------------------------------------------------------------
    // Time (버스트 발생 시간)
    // ------------------------------------------------------------------------
    // 이미터 수명 대비 비율 (0.0 ~ 1.0)
    //
    // 예시 (EmitterDuration = 5초):
    // - Time=0.0 → 0초에 발생
    // - Time=0.5 → 2.5초에 발생
    // - Time=1.0 → 5초에 발생
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "[버스트]", Tooltip = "버스트 시간")
    float Time = 0.0f;

    UParticleBurst() = default;
    UParticleBurst(int32 InCount, float InTime)
        : Count(InCount), CountLow(-1), Time(InTime) {
    }
    UParticleBurst(int32 InCountLow, int32 InCount, float InTime)
        : Count(InCount), CountLow(InCountLow), Time(InTime) {
    }
    ~UParticleBurst() override = default;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle);
};