#include "pch.h"
#include "ParticleModuleSpawn.h"


// ============================================================================
// 생성자
// ============================================================================
UParticleModuleSpawn::UParticleModuleSpawn()
{
    // 기본 스폰율: 20 파티클/초
    Rate.Min = 20.0f;
    Rate.Max = 20.0f;

    // 기본 스폰율 배율: 1.0 (100%)
    RateScale.Min = 1.0f;
    RateScale.Max = 1.0f;
}

// ============================================================================
// Getters
// ============================================================================
const FRawDistribution<float>& UParticleModuleSpawn::GetRate() const { return Rate; }
const FRawDistribution<float>& UParticleModuleSpawn::GetRateScale() const { return RateScale; }
const TArray<UParticleBurst*>& UParticleModuleSpawn::GetBurstList() const { return BurstList; }

float UParticleModuleSpawn::GetMaxSpawnRate() const
{
    // 최대 스폰율 = Rate.Max * RateScale.Max
    return Rate.Max * RateScale.Max;
}

// ============================================================================
// Setters
// ============================================================================
void UParticleModuleSpawn::SetRate(const FRawDistribution<float>& InRate) { Rate = InRate; }
void UParticleModuleSpawn::SetRateMin(float InMin) { Rate.Min = InMin; }
void UParticleModuleSpawn::SetRateMax(float InMax) { Rate.Max = InMax; }
void UParticleModuleSpawn::SetRateScale(const FRawDistribution<float>& InScale) { RateScale = InScale; }
void UParticleModuleSpawn::SetRateScaleMin(float InMin) { RateScale.Min = InMin; }
void UParticleModuleSpawn::SetRateScaleMax(float InMax) { RateScale.Max = InMax; }

// ============================================================================
// 버스트 관리 함수
// ============================================================================
void UParticleModuleSpawn::AddBurst(UParticleBurst* Burst)
{
    BurstList.Add(Burst);
}

void UParticleModuleSpawn::AddBurst(int32 Count, float Time)
{
    BurstList.Add(NewObject<UParticleBurst>());
    BurstList.Last()->Count = Count;
    BurstList.Last()->Time = Time;
}

void UParticleModuleSpawn::AddBurst(int32 CountLow, int32 Count, float Time)
{
    BurstList.Add(NewObject<UParticleBurst>());
    BurstList.Last()->CountLow = CountLow;
    BurstList.Last()->Count = Count;
    BurstList.Last()->Time = Time;
}

void UParticleModuleSpawn::ClearBursts()
{
    BurstList.Empty();
}

int32 UParticleModuleSpawn::GetBurstCount() const
{
    return BurstList.Num();
}

// ============================================================================
// GetSpawnAmount - 프레임당 스폰 개수 계산
// ============================================================================
// 이번 프레임에 생성할 파티클 개수를 계산합니다.
//
// 알고리즘:
// 1. Distribution에서 Rate와 RateScale 값을 가져옴
// 2. 최종 스폰율 계산: SpawnRate = Rate * RateScale
// 3. 이번 프레임의 누적값 계산: NewLeftover = OldLeftover + DeltaTime * SpawnRate
// 4. 정수부가 스폰할 파티클 개수
// 5. 소수부는 다음 프레임으로 이월
//
// 왜 Leftover를 사용하는가?
// - 스폰율이 30이고 60fps면, 프레임당 0.5개 파티클
// - 정수만 스폰 가능하므로 소수부를 누적하여 처리
// - 이렇게 하면 시간이 지나면서 정확한 개수가 스폰됨
//
// 예시 (Rate=30, 60fps):
// - Frame 1: 0 + 0.0167 * 30 = 0.5 → 스폰 0개, 이월 0.5
// - Frame 2: 0.5 + 0.0167 * 30 = 1.0 → 스폰 1개, 이월 0.0
// ============================================================================
float UParticleModuleSpawn::GetSpawnAmount(float EmitterTime, float OldLeftover, float DeltaTime,
                                           int32& OutNumber, float& OutNewLeftover)
{
    // ------------------------------------------------------------------------
    // Step 1: Distribution에서 Rate와 RateScale 값 가져오기
    // ------------------------------------------------------------------------
    // GetValue는 Mode에 따라:
    // - Uniform 모드: Min~Max 사이 랜덤
    // - Curve 모드: EmitterTime에 따른 보간값
    //
    // EmitterTime을 사용하면 시간에 따라 스폰율이 변할 수 있음
    // 예: 초반에는 많이 생성, 후반에는 적게 생성
    // ------------------------------------------------------------------------
    float RateValue = Rate.GetValue(EmitterTime);
    float RateScaleValue = RateScale.GetValue(EmitterTime);

    // ------------------------------------------------------------------------
    // Step 2: 최종 스폰율 계산
    // ------------------------------------------------------------------------
    // SpawnRate = Rate * RateScale
    //
    // 음수 방지: 스폰율은 0 이상이어야 함
    // ------------------------------------------------------------------------
    float SpawnRate = FMath::Max(0.0f, RateValue * RateScaleValue);

    // ------------------------------------------------------------------------
    // Step 3: 이번 프레임의 누적값 계산
    // ------------------------------------------------------------------------
    // NewLeftover = 이전 소수부 + (경과시간 * 스폰율)
    //
    // 이 값은 "이번 프레임까지 스폰했어야 하는 총 파티클 수"를 의미
    // ------------------------------------------------------------------------
    float NewLeftover = OldLeftover + DeltaTime * SpawnRate;

    // ------------------------------------------------------------------------
    // Step 4: 스폰할 파티클 개수 계산
    // ------------------------------------------------------------------------
    // 정수부 = 실제로 스폰할 파티클 개수
    // Floor를 사용하여 내림 처리
    // ------------------------------------------------------------------------
    OutNumber = FMath::FloorToInt(NewLeftover);

    // ------------------------------------------------------------------------
    // Step 5: 다음 프레임으로 이월할 소수부 계산
    // ------------------------------------------------------------------------
    // 소수부 = 아직 스폰하지 못한 나머지
    // 다음 프레임에 누적되어 처리됨
    // ------------------------------------------------------------------------
    OutNewLeftover = NewLeftover - OutNumber;

    return SpawnRate;
}

// ============================================================================
// GetBurstCount - 버스트 스폰 개수 계산
// ============================================================================
// 이번 프레임에 버스트로 생성할 파티클 개수를 계산합니다.
//
// 알고리즘:
// 1. 각 버스트 항목을 순회
// 2. 아직 발사되지 않았고, 현재 시간이 버스트 시간을 지났으면 발사
// 3. CountLow가 설정되어 있으면 랜덤 범위에서 선택
// 4. 발사된 버스트는 BurstFired에 기록하여 중복 방지
//
// 주의:
// - 버스트는 한 번만 발사됨 (루프 이미터에서는 루프마다 리셋 필요)
// - EmitterDuration이 0이면 버스트 시간 계산 불가
// ============================================================================
int32 UParticleModuleSpawn::GetBurstCount(float EmitterTime, float EmitterDuration,
                                          TArray<bool>& BurstFired)
{
    // ------------------------------------------------------------------------
    // Step 0: 버스트 목록이 비어있으면 0 반환
    // ------------------------------------------------------------------------
    if (BurstList.Num() == 0)
    {
        return 0;
    }

    // ------------------------------------------------------------------------
    // Step 1: BurstFired 배열 초기화 (필요시)
    // ------------------------------------------------------------------------
    // BurstFired는 각 버스트의 발사 여부를 추적
    // 처음 호출 시 또는 크기가 맞지 않으면 초기화
    // ------------------------------------------------------------------------
    if (BurstFired.Num() != BurstList.Num())
    {
        BurstFired.SetNum(BurstList.Num());
        for (int32 i = 0; i < BurstFired.Num(); ++i)
        {
            BurstFired[i] = false;
        }
    }

    int32 TotalBurst = 0;

    // ------------------------------------------------------------------------
    // Step 2: 각 버스트 항목 처리
    // ------------------------------------------------------------------------
    for (int32 i = 0; i < BurstList.Num(); ++i)
    {
        const UParticleBurst* Burst = BurstList[i];

        // 이미 발사된 버스트는 건너뜀
        if (BurstFired[i])
        {
            continue;
        }

        // 버스트 발생 시간 계산
        // Time은 0~1 비율이므로 EmitterDuration을 곱함
        // EmitterDuration이 0이면 (무한 루프) Time을 초 단위로 간주
        float BurstTime = (EmitterDuration > 0.0f)
            ? Burst->Time * EmitterDuration
            : Burst->Time;

        // 현재 시간이 버스트 시간을 지났는지 확인
        if (EmitterTime >= BurstTime)
        {
            // 버스트 개수 결정
            int32 BurstCount = Burst->Count;

            // CountLow가 0 이상이면 랜덤 범위 사용
            if (Burst->CountLow >= 0)
            {
                // CountLow ~ Count 범위에서 랜덤 선택
                BurstCount = Burst->CountLow +
                    FMath::FloorToInt(FMath::GetRandZeroOneRange() * (Burst->Count - Burst->CountLow + 1));
                BurstCount = FMath::Clamp(BurstCount, Burst->CountLow, Burst->Count);
            }

            TotalBurst += BurstCount;

            // 발사 완료 기록
            BurstFired[i] = true;
        }
    }

    return TotalBurst;
}
