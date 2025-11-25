#pragma once

#include "ParticleModule.h"
#include "Statistics.h"
#include "UParticleModuleSizeOverLife.generated.h"

struct FBaseParticle;

// ============================================================================
// UParticleModuleSizeOverLife
// ============================================================================
// 파티클의 수명에 따라 크기를 변화시키는 모듈입니다.
//
// 주요 기능:
// 1. RelativeTime (0.0 ~ 1.0) 기반 크기 보간
// 2. Scale 방식 (BaseSize에 곱셈) 사용
// 3. Curve 모드로 복잡한 크기 변화 지원
// 4. 균일/비균일 스케일 지원
//
// 적용 대상:
// - Sprite: 파티클 수명에 따른 크기 변화 (예: 연기 팽창)
// - Beam: T (0=시작점, 1=끝점) 기반 폭 변화 (테이퍼링)
// - Ribbon: T (0=Head, 1=Tail) 기반 폭 변화
//
// 동작 방식:
// - Update()에서 매 프레임 호출
// - Size = BaseSize * ScaleOverLife
// ============================================================================
UCLASS(DisplayName="수명별 크기", Description="파티클의 수명에 따라 크기를 변화시킵니다.")
class UParticleModuleSizeOverLife : public UParticleModule
{
public:
    UParticleModuleSizeOverLife();
    ~UParticleModuleSizeOverLife() = default;

    GENERATED_REFLECTION_BODY()

    // ========================================================================
    // UParticleModule 인터페이스 구현
    // ========================================================================

    // Spawn: 초기 설정 (필요 시)
    void Spawn(FParticleContext& Context, float EmitterTime) override;

    // Update: 매 프레임 크기 업데이트
    void Update(FParticleContext& Context, float DeltaTime) override;

    // ========================================================================
    // Getters
    // ========================================================================
    const FRawDistribution<FVector>& GetScaleOverLife() const { return ScaleOverLife; }
    bool GetUseUniformScale() const { return bUseUniformScale; }

    // ========================================================================
    // Setters
    // ========================================================================
    void SetScaleOverLife(const FRawDistribution<FVector>& InScale) { ScaleOverLife = InScale; }
    void SetUseUniformScale(bool bUniform) { bUseUniformScale = bUniform; }

    // ========================================================================
    // 헬퍼 함수 - Curve 모드 설정
    // ========================================================================

    // 스케일 커브에 키프레임 추가 (Time: 0.0 ~ 1.0)
    void AddScaleKey(float Time, const FVector& Scale);

    // 균일 스케일 커브에 키프레임 추가
    void AddUniformScaleKey(float Time, float Scale);

    // 커브 초기화
    void ClearScaleCurve();

    // ========================================================================
    // 헬퍼 함수 - 간편 설정
    // ========================================================================

    // 시작~끝 스케일 설정 (Uniform 모드)
    void SetScaleFade(const FVector& StartScale, const FVector& EndScale);

    // 균일 스케일 설정
    void SetUniformScaleFade(float StartScale, float EndScale);

    // 성장 (작게 → 크게)
    void SetGrow(float StartScale = 0.0f, float EndScale = 1.0f);

    // 수축 (크게 → 작게)
    void SetShrink(float StartScale = 1.0f, float EndScale = 0.0f);

    // 팽창 후 수축 (작게 → 크게 → 작게)
    void SetPulse(float MinScale = 0.5f, float MaxScale = 1.5f);

    // T값 (0~1)에 따른 스케일 직접 계산 (Beam/Ribbon 렌더링용)
    FVector EvaluateScaleAtTime(float T) const;

    // 균일 스케일 값 직접 계산
    float EvaluateUniformScaleAtTime(float T) const;

private:
    // ========================================================================
    // 크기 관련 프로퍼티
    // ========================================================================

    // ------------------------------------------------------------------------
    // ScaleOverLife (수명별 스케일)
    // ------------------------------------------------------------------------
    // RelativeTime (0.0 ~ 1.0)에 따른 크기 배율입니다.
    //
    // BaseSize에 곱해져서 최종 Size가 됩니다:
    // Size = BaseSize * ScaleOverLife
    //
    // Curve 모드 사용 시:
    // - 키프레임의 InVal이 RelativeTime
    // - 키프레임의 OutVal이 해당 시점의 스케일
    //
    // Uniform 모드 사용 시:
    // - Min = 시작 스케일 (T=0)
    // - Max = 끝 스케일 (T=1)
    // - RelativeTime으로 선형 보간
    //
    // 예시 (연기):
    // - T=0.0: (0.5, 0.5, 0.5) 작게 시작
    // - T=0.5: (1.5, 1.5, 1.5) 팽창
    // - T=1.0: (2.0, 2.0, 2.0) 최대 크기
    //
    // 예시 (불꽃):
    // - T=0.0: (1.0, 1.0, 1.0) 정상 크기
    // - T=1.0: (0.0, 0.0, 0.0) 사라짐
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[크기]", Tooltip="수명에 따른 크기 배율 (BaseSize에 곱해짐)")
    FRawDistribution<FVector> ScaleOverLife{};

    // ------------------------------------------------------------------------
    // bUseUniformScale (균일 스케일)
    // ------------------------------------------------------------------------
    // true: X값만 사용하여 XYZ 모두 동일하게 스케일
    // false: XYZ 개별 스케일 사용
    //
    // 대부분의 파티클은 균일 스케일이 자연스러움
    // 비균일 스케일은 특수 효과(늘어나는 파티클 등)에 사용
    // ------------------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category="[크기]", Tooltip="균일 스케일 사용 (X값으로 XYZ 모두 스케일)")
    bool bUseUniformScale = true;
};
