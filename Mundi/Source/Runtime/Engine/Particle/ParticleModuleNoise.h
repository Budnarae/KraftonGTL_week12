#pragma once

#include "ParticleModule.h"
#include "Statistics.h"
#include "UParticleModuleNoise.generated.h"

struct FBaseParticle;

// ============================================================================
// ENoiseAlgorithm - 노이즈 알고리즘 타입
// ============================================================================
enum class ENoiseAlgorithm : uint8
{
    Perlin,              // Perlin 노이즈 (부드러운 출렁임)
    MidpointDisplacement // 중간 변위 (번개 효과 - 지직거림)
};

// ============================================================================
// UParticleModuleNoise
// ============================================================================
// 파티클의 위치에 노이즈를 적용하는 모듈입니다.
//
// 주요 기능:
// 1. Perlin 노이즈: 부드럽고 연속적인 변위 (연기, 출렁이는 효과)
// 2. Midpoint Displacement: 불규칙한 변위 (번개, 전기 효과)
// 3. 시간에 따른 노이즈 애니메이션
// 4. 수명에 따른 노이즈 강도 조절
//
// 적용 대상:
// - Sprite: 파티클 위치에 직접 노이즈 적용
// - Beam: 빔 세그먼트 변위 계산 (TypeDataBeam에서 호출)
// - Ribbon: 리본 포인트 변위 계산 (TypeDataRibbon에서 호출)
//
// 동작 방식 (Sprite):
// - Update()에서 매 프레임 호출
// - Location += NoiseOffset
//
// 동작 방식 (Beam/Ribbon):
// - CalculateNoiseOffset() 직접 호출
// - 렌더링 시 세그먼트별 변위 계산
// ============================================================================
UCLASS(DisplayName="노이즈", Description="파티클 위치에 노이즈를 적용합니다.")
class UParticleModuleNoise : public UParticleModule
{
public:
    UParticleModuleNoise();
    ~UParticleModuleNoise() = default;

    GENERATED_REFLECTION_BODY()

    // ========================================================================
    // UParticleModule 인터페이스 구현
    // ========================================================================

    // Spawn: 초기 노이즈 시드/오프셋 설정
    void Spawn(FParticleContext& Context, float EmitterTime) override;

    // Update: 매 프레임 노이즈 적용
    void Update(FParticleContext& Context, float DeltaTime) override;

    // ========================================================================
    // Getters
    // ========================================================================
    ENoiseAlgorithm GetNoiseAlgorithm() const { return NoiseAlgorithm; }
    float GetNoiseAmplitude() const { return NoiseAmplitude; }
    float GetNoiseFrequency() const { return NoiseFrequency; }
    float GetNoiseSpeed() const { return NoiseSpeed; }
    float GetJitterFrequency() const { return JitterFrequency; }
    float GetDisplacementDecay() const { return DisplacementDecay; }
    bool GetApplyToLocation() const { return bApplyToLocation; }
    bool GetScaleByLife() const { return bScaleByLife; }

    // ========================================================================
    // Setters
    // ========================================================================
    void SetNoiseAlgorithm(ENoiseAlgorithm Value) { NoiseAlgorithm = Value; }
    void SetNoiseAmplitude(float Value) { NoiseAmplitude = Value; }
    void SetNoiseFrequency(float Value) { NoiseFrequency = Value; }
    void SetNoiseSpeed(float Value) { NoiseSpeed = Value; }
    void SetJitterFrequency(float Value) { JitterFrequency = Value; }
    void SetDisplacementDecay(float Value) { DisplacementDecay = Value; }
    void SetApplyToLocation(bool Value) { bApplyToLocation = Value; }
    void SetScaleByLife(bool Value) { bScaleByLife = Value; }

    // ========================================================================
    // 노이즈 계산 함수 (Beam/Ribbon 렌더링용)
    // ========================================================================

    // T값 (0~1)에 따른 노이즈 오프셋 계산
    // @param T: 위치 비율 (Beam: 0=시작, 1=끝 / Ribbon: 0=Head, 1=Tail)
    // @param Time: 현재 시간 (애니메이션용)
    // @param Right: 변위 기준 오른쪽 벡터
    // @param Up: 변위 기준 위쪽 벡터
    // @return 노이즈 변위 벡터
    FVector CalculateNoiseOffset(float T, float Time, const FVector& Right, const FVector& Up) const;

    // Midpoint Displacement로 번개 포인트 생성
    // @param StartPoint: 시작점
    // @param EndPoint: 끝점
    // @param Depth: 재귀 깊이 (세그먼트 수 = 2^Depth)
    // @param Time: 현재 시간 (시드 계산용)
    // @param OutPoints: 출력 포인트 배열
    void GenerateLightningPoints(
        const FVector& StartPoint,
        const FVector& EndPoint,
        int32 Depth,
        float Time,
        TArray<FVector>& OutPoints
    ) const;

    // ========================================================================
    // 헬퍼 함수 - 프리셋
    // ========================================================================

    // 부드러운 출렁임 (연기, 불꽃)
    void SetupSmoothNoise(float Amplitude = 0.5f, float Frequency = 1.0f, float Speed = 1.0f);

    // 번개 효과
    void SetupLightningNoise(float Amplitude = 1.0f, float JitterFreq = 20.0f);

    // 진동 효과 (빠르고 작은 노이즈)
    void SetupVibrationNoise(float Amplitude = 0.1f, float Frequency = 10.0f);

private:
    // ========================================================================
    // 노이즈 설정 프로퍼티
    // ========================================================================

    // ------------------------------------------------------------------------
    // NoiseAlgorithm (노이즈 알고리즘)
    // ------------------------------------------------------------------------
    // Perlin: 부드럽고 연속적인 노이즈
    //         - 연기, 구름, 물결 효과에 적합
    //         - 시간에 따라 부드럽게 변화
    //
    // MidpointDisplacement: 불규칙한 분기 노이즈
    //         - 번개, 전기, 균열 효과에 적합
    //         - JitterFrequency에 따라 지직거림
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[노이즈]", Tooltip="노이즈 알고리즘")
    ENoiseAlgorithm NoiseAlgorithm = ENoiseAlgorithm::Perlin;

    // ------------------------------------------------------------------------
    // NoiseAmplitude (노이즈 진폭)
    // ------------------------------------------------------------------------
    // 노이즈 변위의 최대 크기입니다.
    // 단위는 월드 유닛입니다.
    //
    // 예시:
    // - 0.1: 미세한 떨림
    // - 1.0: 눈에 띄는 흔들림
    // - 10.0: 큰 변위 (번개 등)
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[노이즈]", Tooltip="노이즈 변위 크기")
    float NoiseAmplitude = 1.0f;

    // ------------------------------------------------------------------------
    // NoiseFrequency (노이즈 주파수)
    // ------------------------------------------------------------------------
    // Perlin 노이즈의 공간 주파수입니다.
    // 높을수록 더 촘촘한 패턴이 생성됩니다.
    //
    // 예시:
    // - 0.5: 넓고 부드러운 곡선
    // - 2.0: 촘촘한 물결
    // - 10.0: 매우 세밀한 패턴
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[노이즈]", Tooltip="노이즈 공간 주파수 (Perlin)")
    float NoiseFrequency = 1.0f;

    // ------------------------------------------------------------------------
    // NoiseSpeed (노이즈 애니메이션 속도)
    // ------------------------------------------------------------------------
    // Perlin 노이즈의 시간 변화 속도입니다.
    // 높을수록 빠르게 출렁입니다.
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[노이즈]", Tooltip="노이즈 애니메이션 속도 (Perlin)")
    float NoiseSpeed = 1.0f;

    // ------------------------------------------------------------------------
    // JitterFrequency (지터 주파수)
    // ------------------------------------------------------------------------
    // MidpointDisplacement의 변화 빈도입니다.
    // 초당 몇 번 패턴이 바뀌는지를 나타냅니다.
    //
    // 예시:
    // - 5.0: 느리게 변화 (유령 효과)
    // - 20.0: 빠르게 지직거림 (번개)
    // - 60.0: 매우 빠른 변화 (전기)
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[노이즈]", Tooltip="패턴 변화 빈도 (MidpointDisplacement)")
    float JitterFrequency = 20.0f;

    // ------------------------------------------------------------------------
    // DisplacementDecay (변위 감쇠)
    // ------------------------------------------------------------------------
    // MidpointDisplacement에서 재귀 단계별 감쇠율입니다.
    // 0.0: 감쇠 없음 (모든 단계 동일한 변위)
    // 0.5: 절반씩 감소 (기본값)
    // 1.0: 빠르게 감소
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[노이즈]", Tooltip="재귀 단계별 변위 감쇠 (MidpointDisplacement)")
    float DisplacementDecay = 0.5f;

    // ========================================================================
    // 적용 설정 프로퍼티
    // ========================================================================

    // ------------------------------------------------------------------------
    // bApplyToLocation (위치에 적용)
    // ------------------------------------------------------------------------
    // true: Update()에서 파티클 Location에 직접 노이즈 적용
    // false: 노이즈 계산만 (Beam/Ribbon에서 직접 호출)
    //
    // Beam/Ribbon은 렌더링 시점에 노이즈를 적용하므로
    // 이 옵션을 false로 설정하고 CalculateNoiseOffset()을 사용
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[적용]", Tooltip="파티클 위치에 직접 노이즈 적용")
    bool bApplyToLocation = true;

    // ------------------------------------------------------------------------
    // bScaleByLife (수명에 따른 스케일)
    // ------------------------------------------------------------------------
    // true: RelativeTime에 따라 노이즈 강도 감소
    //       (파티클이 오래될수록 노이즈 감소)
    // false: 일정한 노이즈 강도
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[적용]", Tooltip="수명에 따라 노이즈 강도 감소")
    bool bScaleByLife = false;

    // ========================================================================
    // 내부 데이터
    // ========================================================================

    // 전역 시간 누적 (Update 간 유지)
    mutable float AccumulatedTime = 0.0f;
};
