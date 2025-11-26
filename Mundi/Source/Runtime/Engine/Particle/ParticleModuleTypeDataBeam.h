#pragma once

#include "ParticleModuleTypeDataBase.h"
#include "Vector.h"
#include "UEContainer.h"
#include "UParticleModuleTypeDataBeam.generated.h"

/**
 * Beam method - how the beam endpoints are determined
 */
enum class EBeamMethod : uint8
{
    Distance,   // Beam extends in a direction for a set distance
    Target,     // Beam connects to a target point
    Source      // Beam connects from source to emitter
};

/**
 * Beam noise type
 */
enum class EBeamNoiseType : uint8
{
    None,       // No noise
    PerPoint,   // Noise applied per control point
    PerSegment  // Noise applied per segment
};

/**
 * Beam noise algorithm
 */
enum class EBeamNoiseAlgorithm : uint8
{
    MidpointDisplacement,   // 중간 변위 알고리즘 (번개 효과 - 지직거림)
    PerlinNoise             // Perlin 노이즈 (부드러운 출렁임)
};

// Forward declarations
struct FBeamNoiseParams;
struct FBeamWidthParams;

/**
 * TypeData module for Beam emitters
 * Defines how beams are generated and rendered
 */
UCLASS(DisplayName="파티클 타입 데이터 빔", Description="빔 에미터의 설정을 정의합니다.")
class UParticleModuleTypeDataBeam : public UParticleModuleTypeDataBase
{
public:
    UParticleModuleTypeDataBeam();
    virtual ~UParticleModuleTypeDataBeam() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // UParticleModuleTypeDataBase interface
    virtual EDynamicEmitterType GetEmitterType() const override { return EDynamicEmitterType::EDET_Beam; }
    virtual bool RequiresSpecialRendering() const override { return true; }

    // Getters
    EBeamMethod GetBeamMethod() const { return BeamMethod; }
    int32 GetSegmentCount() const { return SegmentCount; }
    float GetBeamWidth() const { return BeamWidth; }
    float GetBeamLength() const { return BeamLength; }
    FVector GetSourcePoint() const { return SourcePoint; }
    FVector GetTargetPoint() const { return TargetPoint; }
    float GetNoiseAmplitude() const { return NoiseAmplitude; }
    float GetNoiseFrequency() const { return NoiseFrequency; }
    float GetTextureRepeat() const { return TextureRepeat; }
    bool GetTaperBeam() const { return bTaperBeam; }
    float GetTaperFactor() const { return TaperFactor; }
    FVector GetBeamDirection() const { return BeamDirection; }
    FVector4 GetBeamColor() const { return BeamColor; }
    float GetJitterFrequency() const { return JitterFrequency; }
    float GetDisplacementDecay() const { return DisplacementDecay; }
    float GetGlowIntensity() const { return GlowIntensity; }
    bool GetUseTexture() const { return bUseTexture; }
    EBeamNoiseAlgorithm GetNoiseAlgorithm() const { return NoiseAlgorithm; }

    // Setters
    void SetBeamMethod(EBeamMethod Value) { BeamMethod = Value; }
    void SetSegmentCount(int32 Value) { SegmentCount = Value; }
    void SetBeamWidth(float Value) { BeamWidth = Value; }
    void SetBeamLength(float Value) { BeamLength = Value; }
    void SetSourcePoint(const FVector& Value) { SourcePoint = Value; }
    void SetTargetPoint(const FVector& Value) { TargetPoint = Value; }
    void SetNoiseAmplitude(float Value) { NoiseAmplitude = Value; }
    void SetNoiseFrequency(float Value) { NoiseFrequency = Value; }
    void SetTextureRepeat(float Value) { TextureRepeat = Value; }
    void SetTaperBeam(bool Value) { bTaperBeam = Value; }
    void SetTaperFactor(float Value) { TaperFactor = Value; }
    void SetBeamDirection(const FVector& Value) { BeamDirection = Value; }
    void SetBeamColor(const FVector4& Value) { BeamColor = Value; }
    void SetJitterFrequency(float Value) { JitterFrequency = Value; }
    void SetDisplacementDecay(float Value) { DisplacementDecay = Value; }
    void SetGlowIntensity(float Value) { GlowIntensity = Value; }
    void SetUseTexture(bool Value) { bUseTexture = Value; }
    void SetNoiseAlgorithm(EBeamNoiseAlgorithm Value) { NoiseAlgorithm = Value; }

    // Calculate beam points for rendering
    // NoiseParams: if not null, apply noise with these parameters; if null, no noise
    // WidthParams: if not null, apply width with these parameters; if null, use internal BeamWidth
    void CalculateBeamPoints(
        const FVector& EmitterLocation,
        TArray<FVector>& OutPoints,
        TArray<float>& OutWidths,
        float Time = 0.0f,
        const FBeamNoiseParams* NoiseParams = nullptr,
        const FBeamWidthParams* WidthParams = nullptr
    ) const;

private:
    // Beam configuration
    UPROPERTY(EditAnywhere, Category="Beam|Method")
    EBeamMethod BeamMethod = EBeamMethod::Distance;

    // Number of segments in the beam (more = smoother curves)
    UPROPERTY(EditAnywhere, Category="Beam|Segments")
    int32 SegmentCount = 10;

    // Width of the beam
    UPROPERTY(EditAnywhere, Category="Beam|Appearance")
    float BeamWidth = 1.0f;

    // Length of beam (for Distance method)
    UPROPERTY(EditAnywhere, Category="Beam|Method")
    float BeamLength = 100.0f;

    // Source point (local space)
    UPROPERTY(EditAnywhere, Category="Beam|Endpoints")
    FVector SourcePoint = FVector::Zero();

    // Target point (local/world space depending on settings)
    UPROPERTY(EditAnywhere, Category="Beam|Endpoints")
    FVector TargetPoint = FVector(100.0f, 0.0f, 0.0f);

    // Noise settings
    UPROPERTY(EditAnywhere, Category="Beam|Noise")
    EBeamNoiseType NoiseType = EBeamNoiseType::None;

    UPROPERTY(EditAnywhere, Category="Beam|Noise")
    float NoiseAmplitude = 0.0f;

    UPROPERTY(EditAnywhere, Category="Beam|Noise")
    float NoiseFrequency = 1.0f;

    // Noise algorithm (MidpointDisplacement = 번개, PerlinNoise = 부드러운 출렁임)
    UPROPERTY(EditAnywhere, Category="Beam|Noise")
    EBeamNoiseAlgorithm NoiseAlgorithm = EBeamNoiseAlgorithm::MidpointDisplacement;

    // Texture settings
    UPROPERTY(EditAnywhere, Category="Beam|Texture")
    float TextureRepeat = 1.0f;

    // Taper (beam gets thinner at the end)
    UPROPERTY(EditAnywhere, Category="Beam|Appearance")
    bool bTaperBeam = false;

    UPROPERTY(EditAnywhere, Category="Beam|Appearance")
    float TaperFactor = 0.5f;

    // Direction for Distance method
    UPROPERTY(EditAnywhere, Category="Beam|Method")
    FVector BeamDirection = FVector(1.0f, 0.0f, 0.0f);

    // Beam color (RGBA)
    UPROPERTY(EditAnywhere, Category="Beam|Appearance")
    FVector4 BeamColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

    // Jitter frequency - how many times per second the lightning changes
    UPROPERTY(EditAnywhere, Category="Beam|Animation")
    float JitterFrequency = 20.0f;

    // Displacement decay - how much displacement decreases at each recursion level (0.0 ~ 1.0)
    UPROPERTY(EditAnywhere, Category="Beam|Noise")
    float DisplacementDecay = 0.5f;

    // Glow intensity - multiplier for additive blending effect
    UPROPERTY(EditAnywhere, Category="Beam|Appearance")
    float GlowIntensity = 1.0f;

    // Use texture instead of solid color
    UPROPERTY(EditAnywhere, Category="Beam|Texture")
    bool bUseTexture = false;
};
