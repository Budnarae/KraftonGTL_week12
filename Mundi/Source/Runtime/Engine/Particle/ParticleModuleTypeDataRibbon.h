#pragma once

#include "ParticleModuleTypeDataBase.h"
#include "Vector.h"
#include "UEContainer.h"
#include "Color.h"
#include "UParticleModuleTypeDataRibbon.generated.h"

// Forward declarations
struct FParticleEmitterInstance;

/**
 * Ribbon facing mode - how the ribbon faces the camera
 */
enum class ERibbonFacingMode : uint8
{
    FaceCamera,     // Always face camera (billboard)
    FaceVelocity,   // Face velocity direction
    Custom          // Use custom normal
};

/**
 * Payload structure for ribbon particles
 * Each particle stores its spawn time for sorting and linking
 */
struct FRibbonPayload
{
    // Spawn time for sorting particles in correct order
    float SpawnTime;

    // Previous particle index in the chain (-1 if head)
    int32 SourceIndex;

    // Width at this point
    float Width;

    void Initialize(float InSpawnTime, int32 InSourceIndex, float InWidth)
    {
        SpawnTime = InSpawnTime;
        SourceIndex = InSourceIndex;
        Width = InWidth;
    }
};

/**
 * Helper structure for sorted ribbon particle data
 */
struct FRibbonParticleData
{
    FVector Location;
    FLinearColor Color;
    float Width;
    float SpawnTime;
    int32 OriginalIndex;
};

/**
 * TypeData module for Ribbon/Trail emitters
 * Uses multi-particle approach where each particle is a point in the ribbon
 */
UCLASS(DisplayName="파티클 타입 데이터 리본", Description="리본/트레일 에미터의 설정을 정의합니다.")
class UParticleModuleTypeDataRibbon : public UParticleModuleTypeDataBase
{
public:
    UParticleModuleTypeDataRibbon();
    virtual ~UParticleModuleTypeDataRibbon() = default;

    GENERATED_REFLECTION_BODY()

    // UParticleModule interface
    virtual void Spawn(FParticleContext& Context, float EmitterTime) override;
    virtual void Update(FParticleContext& Context, float DeltaTime) override;

    // UParticleModuleTypeDataBase interface
    virtual EDynamicEmitterType GetEmitterType() const override { return EDynamicEmitterType::EDET_Ribbon; }
    virtual bool RequiresSpecialRendering() const override { return true; }

    // Getters
    ERibbonFacingMode GetFacingMode() const { return FacingMode; }
    float GetRibbonWidth() const { return RibbonWidth; }
    bool GetTaperRibbon() const { return bTaperRibbon; }
    float GetTaperFactor() const { return TaperFactor; }
    FVector4 GetRibbonColor() const { return RibbonColor; }
    float GetTextureRepeat() const { return TextureRepeat; }
    bool GetUseTexture() const { return bUseTexture; }
    int32 GetMaxParticlesPerRibbon() const { return MaxParticlesPerRibbon; }

    // Setters
    void SetFacingMode(ERibbonFacingMode Value) { FacingMode = Value; }
    void SetRibbonWidth(float Value) { RibbonWidth = Value; }
    void SetTaperRibbon(bool Value) { bTaperRibbon = Value; }
    void SetTaperFactor(float Value) { TaperFactor = Value; }
    void SetRibbonColor(const FVector4& Value) { RibbonColor = Value; }
    void SetTextureRepeat(float Value) { TextureRepeat = Value; }
    void SetUseTexture(bool Value) { bUseTexture = Value; }
    void SetMaxParticlesPerRibbon(int32 Value) { MaxParticlesPerRibbon = Value; }

    /**
     * Build ribbon geometry from all active particles in the emitter
     * Called from CollectMeshBatches during rendering
     *
     * @param EmitterInstance - The emitter instance containing particles
     * @param EmitterLocation - World location of the emitter
     * @param OutPoints - Output array of ribbon points (sorted by spawn time)
     * @param OutWidths - Output array of widths at each point
     * @param OutColors - Output array of colors at each point
     */
    void BuildRibbonFromParticles(
        FParticleEmitterInstance* EmitterInstance,
        const FVector& EmitterLocation,
        TArray<FVector>& OutPoints,
        TArray<float>& OutWidths,
        TArray<FLinearColor>& OutColors
    ) const;

    // Get payload from particle
    FRibbonPayload* GetPayload(FBaseParticle* Particle) const;

private:
    // How the ribbon faces the camera
    UPROPERTY(EditAnywhere, Category="Ribbon|Facing")
    ERibbonFacingMode FacingMode = ERibbonFacingMode::FaceCamera;

    // Base width of the ribbon
    UPROPERTY(EditAnywhere, Category="Ribbon|Appearance")
    float RibbonWidth = 5.0f;

    // Taper (ribbon gets thinner at the tail)
    UPROPERTY(EditAnywhere, Category="Ribbon|Appearance")
    bool bTaperRibbon = true;

    UPROPERTY(EditAnywhere, Category="Ribbon|Appearance")
    float TaperFactor = 0.0f;

    // Ribbon color tint (RGBA)
    UPROPERTY(EditAnywhere, Category="Ribbon|Appearance")
    FVector4 RibbonColor = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

    // Texture repeat along ribbon length
    UPROPERTY(EditAnywhere, Category="Ribbon|Texture")
    float TextureRepeat = 1.0f;

    // Use texture instead of solid color
    UPROPERTY(EditAnywhere, Category="Ribbon|Texture")
    bool bUseTexture = false;

    // Maximum particles per ribbon (for sorting performance)
    UPROPERTY(EditAnywhere, Category="Ribbon|Performance")
    int32 MaxParticlesPerRibbon = 100;
};
