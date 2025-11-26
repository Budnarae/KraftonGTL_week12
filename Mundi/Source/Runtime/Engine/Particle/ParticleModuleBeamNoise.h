#pragma once

#include "ParticleModule.h"
#include "ParticleModuleTypeDataBeam.h"  // EBeamNoiseAlgorithm 사용
#include "UParticleModuleBeamNoise.generated.h"

/**
 * Beam noise parameters - passed to CalculateBeamPoints
 */
struct FBeamNoiseParams
{
    EBeamNoiseAlgorithm Algorithm = EBeamNoiseAlgorithm::MidpointDisplacement;
    float Amplitude = 10.0f;
    float Frequency = 1.0f;
    float JitterFrequency = 20.0f;
    float DisplacementDecay = 0.5f;
};

/**
 * Module that adds noise/lightning effects to beam emitters.
 * If this module is present, it overrides TypeDataBeam's noise settings.
 */
UCLASS(DisplayName="빔 노이즈", Description="빔에 번개/노이즈 효과를 추가합니다.")
class UParticleModuleBeamNoise : public UParticleModule
{
public:
    UParticleModuleBeamNoise();
    virtual ~UParticleModuleBeamNoise() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // Get all noise parameters as a struct (for passing to CalculateBeamPoints)
    FBeamNoiseParams GetParams() const
    {
        FBeamNoiseParams Params;
        Params.Algorithm = NoiseAlgorithm;
        Params.Amplitude = NoiseAmplitude;
        Params.Frequency = NoiseFrequency;
        Params.JitterFrequency = JitterFrequency;
        Params.DisplacementDecay = DisplacementDecay;
        return Params;
    }

    // Individual Getters
    EBeamNoiseAlgorithm GetNoiseAlgorithm() const { return NoiseAlgorithm; }
    float GetNoiseAmplitude() const { return NoiseAmplitude; }
    float GetNoiseFrequency() const { return NoiseFrequency; }
    float GetJitterFrequency() const { return JitterFrequency; }
    float GetDisplacementDecay() const { return DisplacementDecay; }

    // Setters
    void SetNoiseAlgorithm(EBeamNoiseAlgorithm Value) { NoiseAlgorithm = Value; }
    void SetNoiseAmplitude(float Value) { NoiseAmplitude = Value; }
    void SetNoiseFrequency(float Value) { NoiseFrequency = Value; }
    void SetJitterFrequency(float Value) { JitterFrequency = Value; }
    void SetDisplacementDecay(float Value) { DisplacementDecay = Value; }

private:
    // Noise algorithm (MidpointDisplacement = 번개, PerlinNoise = 부드러운 출렁임)
    UPROPERTY(EditAnywhere, Category="Noise|Algorithm")
    EBeamNoiseAlgorithm NoiseAlgorithm = EBeamNoiseAlgorithm::MidpointDisplacement;

    // Noise amplitude - how much the beam deviates from straight line
    UPROPERTY(EditAnywhere, Category="Noise|Amplitude")
    float NoiseAmplitude = 10.0f;

    // Noise frequency - how many waves along the beam
    UPROPERTY(EditAnywhere, Category="Noise|Frequency")
    float NoiseFrequency = 1.0f;

    // Jitter frequency - how many times per second the lightning changes (for MidpointDisplacement)
    UPROPERTY(EditAnywhere, Category="Noise|Animation")
    float JitterFrequency = 20.0f;

    // Displacement decay - how much displacement decreases at each recursion level (0.0 ~ 1.0)
    UPROPERTY(EditAnywhere, Category="Noise|Decay")
    float DisplacementDecay = 0.5f;
};
