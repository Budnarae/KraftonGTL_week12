#pragma once

#include "ParticleModule.h"
#include "Color.h"
#include "UParticleModuleBeamColorOverLength.generated.h"

/**
 * Beam color parameters - passed to beam rendering
 * T = 0.0 (start point) to 1.0 (end point) along the beam length
 */
struct FBeamColorParams
{
    FLinearColor StartColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);  // Color at T=0 (source)
    FLinearColor EndColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);    // Color at T=1 (target)
    bool bEnabled = true;

    // Evaluate color at position T (0=start, 1=end)
    FLinearColor EvaluateAtT(float T) const
    {
        T = FMath::Clamp(T, 0.0f, 1.0f);
        return FLinearColor(
            FMath::Lerp(StartColor.R, EndColor.R, T),
            FMath::Lerp(StartColor.G, EndColor.G, T),
            FMath::Lerp(StartColor.B, EndColor.B, T),
            FMath::Lerp(StartColor.A, EndColor.A, T)
        );
    }
};

/**
 * Module that controls beam color gradient along its length.
 * Unlike ColorOverLife (time-based), this is position-based:
 * - T=0: Start point (source) color
 * - T=1: End point (target) color
 *
 * This creates spatial gradients like lightning that fades from bright to dim,
 * or energy beams that change color along their path.
 */
UCLASS(DisplayName="빔 길이별 색상", Description="빔의 시작점에서 끝점까지 색상 그라데이션을 적용합니다.")
class UParticleModuleBeamColorOverLength : public UParticleModule
{
public:
    UParticleModuleBeamColorOverLength();
    ~UParticleModuleBeamColorOverLength() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // Get all color parameters as a struct (for passing to beam rendering)
    FBeamColorParams GetParams() const
    {
        FBeamColorParams Params;
        Params.StartColor = StartColor;
        Params.EndColor = EndColor;
        Params.bEnabled = bActive;
        return Params;
    }

    // Evaluate color at position T (0=start, 1=end)
    FLinearColor EvaluateColorAtT(float T) const
    {
        T = FMath::Clamp(T, 0.0f, 1.0f);
        return FLinearColor(
            FMath::Lerp(StartColor.R, EndColor.R, T),
            FMath::Lerp(StartColor.G, EndColor.G, T),
            FMath::Lerp(StartColor.B, EndColor.B, T),
            FMath::Lerp(StartColor.A, EndColor.A, T)
        );
    }

    // Getters
    FLinearColor GetStartColor() const { return StartColor; }
    FLinearColor GetEndColor() const { return EndColor; }

    // Setters
    void SetStartColor(const FLinearColor& Value) { StartColor = Value; }
    void SetEndColor(const FLinearColor& Value) { EndColor = Value; }

private:
    // Color at the start (source, T=0) of the beam
    UPROPERTY(EditAnywhere, Category="Color|Gradient")
    FLinearColor StartColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Color at the end (target, T=1) of the beam
    UPROPERTY(EditAnywhere, Category="Color|Gradient")
    FLinearColor EndColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
};
