#pragma once

#include "ParticleModule.h"
#include "Color.h"
#include "UParticleModuleRibbonColorOverLength.generated.h"

/**
 * Ribbon color parameters - passed to ribbon rendering
 * T = 0.0 (tail/oldest particle) to 1.0 (head/newest particle) along the ribbon
 */
struct FRibbonColorParams
{
    FLinearColor HeadColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);  // Color at T=1 (head/newest)
    FLinearColor TailColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);  // Color at T=0 (tail/oldest)
    bool bEnabled = true;

    // Evaluate color at position T (0=tail, 1=head)
    FLinearColor EvaluateAtT(float T) const
    {
        if (!bEnabled)
            return HeadColor;

        T = FMath::Clamp(T, 0.0f, 1.0f);
        return FLinearColor(
            FMath::Lerp(TailColor.R, HeadColor.R, T),
            FMath::Lerp(TailColor.G, HeadColor.G, T),
            FMath::Lerp(TailColor.B, HeadColor.B, T),
            FMath::Lerp(TailColor.A, HeadColor.A, T)
        );
    }
};

/**
 * Module that controls ribbon color gradient along its length.
 * T = 0.0 (tail/oldest particle) to 1.0 (head/newest particle)
 *
 * Unlike ColorOverLife (time-based), this is position-based:
 * - T=0: Tail color (oldest particles, end of trail)
 * - T=1: Head color (newest particles, start of trail)
 *
 * This creates spatial gradients like trails that fade out toward the tail,
 * or color transitions along the ribbon path.
 */
UCLASS(DisplayName="리본 길이별 색상", Description="리본의 머리에서 꼬리까지 색상 그라데이션을 적용합니다.")
class UParticleModuleRibbonColorOverLength : public UParticleModule
{
public:
    UParticleModuleRibbonColorOverLength();
    ~UParticleModuleRibbonColorOverLength() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // Get all color parameters as a struct (for passing to ribbon rendering)
    FRibbonColorParams GetParams() const
    {
        FRibbonColorParams Params;
        Params.HeadColor = HeadColor;
        Params.TailColor = TailColor;
        Params.bEnabled = bActive;
        return Params;
    }

    // Evaluate color at position T (0=tail, 1=head)
    FLinearColor EvaluateColorAtT(float T) const
    {
        if (!bActive)
            return HeadColor;

        T = FMath::Clamp(T, 0.0f, 1.0f);
        return FLinearColor(
            FMath::Lerp(TailColor.R, HeadColor.R, T),
            FMath::Lerp(TailColor.G, HeadColor.G, T),
            FMath::Lerp(TailColor.B, HeadColor.B, T),
            FMath::Lerp(TailColor.A, HeadColor.A, T)
        );
    }

    // Getters
    FLinearColor GetHeadColor() const { return HeadColor; }
    FLinearColor GetTailColor() const { return TailColor; }

    // Setters
    void SetHeadColor(const FLinearColor& Value) { HeadColor = Value; }
    void SetTailColor(const FLinearColor& Value) { TailColor = Value; }

private:
    // Color at the head (newest particle, T=1) of the ribbon
    UPROPERTY(EditAnywhere, Category="Color|Gradient")
    FLinearColor HeadColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Color at the tail (oldest particle, T=0) of the ribbon
    // Default: transparent for fade-out effect
    UPROPERTY(EditAnywhere, Category="Color|Gradient")
    FLinearColor TailColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);
};
