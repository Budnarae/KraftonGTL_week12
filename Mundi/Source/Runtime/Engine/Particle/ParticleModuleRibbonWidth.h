#pragma once

#include "ParticleModule.h"
#include "UParticleModuleRibbonWidth.generated.h"

/**
 * Ribbon taper method - how width changes along the ribbon
 * T = 0.0 (tail/oldest) to 1.0 (head/newest)
 */
enum class ERibbonTaperMethod : uint8
{
    None = 0,       // No taper - uniform width
    Linear = 1,     // Linear interpolation from tail to head
    Sine = 2,       // Sine curve - thinnest at tail, thickest at head
    EaseIn = 3,     // Ease in - starts thin, accelerates to thick
    EaseOut = 4     // Ease out - starts thick, thins toward tail
};

/**
 * Ribbon width parameters - passed to BuildRibbonFromParticles
 * Scale values are multiplied with TypeDataRibbon's RibbonWidth
 */
struct FRibbonWidthParams
{
    float HeadWidthScale = 1.0f;   // Scale at head (T=1, newest)
    float TailWidthScale = 0.1f;   // Scale at tail (T=0, oldest)
    ERibbonTaperMethod TaperMethod = ERibbonTaperMethod::Linear;
    bool bEnabled = true;

    // Evaluate width scale at position T (0=tail, 1=head)
    float EvaluateAtT(float T) const
    {
        if (!bEnabled || TaperMethod == ERibbonTaperMethod::None)
            return HeadWidthScale;

        T = FMath::Clamp(T, 0.0f, 1.0f);

        switch (TaperMethod)
        {
        case ERibbonTaperMethod::Linear:
            return FMath::Lerp(TailWidthScale, HeadWidthScale, T);

        case ERibbonTaperMethod::Sine:
            {
                // Sine curve: thin at tail, thick at head
                float SinT = std::sin(T * 3.14159265f * 0.5f);
                return FMath::Lerp(TailWidthScale, HeadWidthScale, SinT);
            }

        case ERibbonTaperMethod::EaseIn:
            {
                // Quadratic ease in
                float EaseT = T * T;
                return FMath::Lerp(TailWidthScale, HeadWidthScale, EaseT);
            }

        case ERibbonTaperMethod::EaseOut:
            {
                // Quadratic ease out
                float EaseT = 1.0f - (1.0f - T) * (1.0f - T);
                return FMath::Lerp(TailWidthScale, HeadWidthScale, EaseT);
            }

        default:
            return HeadWidthScale;
        }
    }
};

/**
 * Module that controls ribbon width and tapering along its length.
 * T = 0.0 (tail/oldest particle) to 1.0 (head/newest particle)
 *
 * If this module is not present, TypeDataRibbon will use its internal settings.
 */
UCLASS(DisplayName="리본 너비", Description="리본의 너비와 테이퍼링을 제어합니다.")
class UParticleModuleRibbonWidth : public UParticleModule
{
public:
    UParticleModuleRibbonWidth();
    virtual ~UParticleModuleRibbonWidth() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // Get all width parameters as a struct
    FRibbonWidthParams GetParams() const
    {
        FRibbonWidthParams Params;
        Params.HeadWidthScale = HeadWidthScale;
        Params.TailWidthScale = TailWidthScale;
        Params.TaperMethod = TaperMethod;
        Params.bEnabled = bActive;
        return Params;
    }

    // Getters
    float GetHeadWidthScale() const { return HeadWidthScale; }
    float GetTailWidthScale() const { return TailWidthScale; }
    ERibbonTaperMethod GetTaperMethod() const { return TaperMethod; }

    // Setters
    void SetHeadWidthScale(float Value) { HeadWidthScale = Value; }
    void SetTailWidthScale(float Value) { TailWidthScale = Value; }
    void SetTaperMethod(ERibbonTaperMethod Value) { TaperMethod = Value; }

private:
    // Width scale at the head (newest particle, T=1)
    UPROPERTY(EditAnywhere, Category="Width|Size")
    float HeadWidthScale = 1.0f;

    // Width scale at the tail (oldest particle, T=0)
    UPROPERTY(EditAnywhere, Category="Width|Size")
    float TailWidthScale = 0.1f;

    // How to interpolate width along the ribbon
    UPROPERTY(EditAnywhere, Category="Width|Taper")
    ERibbonTaperMethod TaperMethod = ERibbonTaperMethod::Linear;
};
