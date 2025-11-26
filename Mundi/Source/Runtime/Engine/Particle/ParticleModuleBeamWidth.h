#pragma once

#include "ParticleModule.h"
#include "UParticleModuleBeamWidth.generated.h"

/**
 * Taper method - how width interpolates between start and end
 * 0 = None:    No taper - use StartWidthScale throughout
 * 1 = Linear:  Linear interpolation from Start to End
 * 2 = Sine:    Sine curve - thinnest at edges, thickest in the middle (번개 효과)
 * 3 = EaseOut: Ease out - starts thick, thins toward end
 */
enum class ETaperMethod : uint8
{
    None = 0,       // No taper - use StartWidth throughout
    Linear = 1,     // Linear interpolation from StartWidth to EndWidth
    Sine = 2,       // Sine curve - thickest in the middle
    EaseOut = 3     // Ease out - starts thick, thins toward end
};

/**
 * Beam width parameters - passed to CalculateBeamPoints
 * Scale values are multiplied with TypeDataBeam's BeamWidth
 */
struct FBeamWidthParams
{
    float StartWidthScale = 1.0f;  // Scale factor at start (0.0 ~ 2.0+)
    float EndWidthScale = 1.0f;    // Scale factor at end (0.0 ~ 2.0+)
    int32 TaperMethodInt = 2;      // 0=None, 1=Linear, 2=Sine, 3=EaseOut (default: Sine)
};

/**
 * Module that controls beam width and tapering.
 * If this module is not present, TypeDataBeam will use its internal BeamWidth as fallback.
 */
UCLASS(DisplayName="빔 너비", Description="빔의 너비와 테이퍼링을 제어합니다.")
class UParticleModuleBeamWidth : public UParticleModule
{
public:
    UParticleModuleBeamWidth();
    virtual ~UParticleModuleBeamWidth() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // Get all width parameters as a struct (for passing to CalculateBeamPoints)
    FBeamWidthParams GetParams() const
    {
        FBeamWidthParams Params;
        Params.StartWidthScale = StartWidthScale;
        Params.EndWidthScale = EndWidthScale;
        Params.TaperMethodInt = TaperMethod;
        return Params;
    }

    // Getters
    float GetStartWidthScale() const { return StartWidthScale; }
    float GetEndWidthScale() const { return EndWidthScale; }
    int32 GetTaperMethod() const { return TaperMethod; }
    bool IsEnabled() const { return bEnabled; }

    // Setters
    void SetStartWidthScale(float Value) { StartWidthScale = Value; }
    void SetEndWidthScale(float Value) { EndWidthScale = Value; }
    void SetTaperMethod(int32 Value) { TaperMethod = Value; }
    void SetEnabled(bool Value) { bEnabled = Value; }

private:
    // Enable/disable width control
    UPROPERTY(EditAnywhere, Category="Width|General")
    bool bEnabled = true;

    // Width scale at the start (source) of the beam (multiplied with TypeDataBeam's BeamWidth)
    UPROPERTY(EditAnywhere, Category="Width|Size")
    float StartWidthScale = 1.0f;

    // Width scale at the end (target) of the beam (multiplied with TypeDataBeam's BeamWidth)
    UPROPERTY(EditAnywhere, Category="Width|Size")
    float EndWidthScale = 1.0f;

    // How to interpolate width along the beam (0=None, 1=Linear, 2=Sine, 3=EaseOut)
    // Sine(2)가 번개 효과에 적합 - 양쪽 끝이 가늘고 중간이 두꺼움
    UPROPERTY(EditAnywhere, Category="Width|Taper")
    int32 TaperMethod = 2;  // Default: Sine
};
