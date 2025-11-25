#include "pch.h"
#include "ParticleModuleTypeDataRibbon.h"
#include "ParticleData.h"
#include "ParticleEmitterInstance.h"
#include <algorithm>

UParticleModuleTypeDataRibbon::UParticleModuleTypeDataRibbon()
    : UParticleModuleTypeDataBase()
{
    // Set payload size for ribbon connection data
    PayloadSize = sizeof(FRibbonPayload);
}

void UParticleModuleTypeDataRibbon::Spawn(FParticleContext& Context, float EmitterTime)
{
    FRibbonPayload* Payload = GetPayload(Context.Particle);
    if (Payload)
    {
        // Store spawn time for sorting during rendering
        // SourceIndex will be determined after sorting by SpawnTime
        // Note: Particle->Location is already in world coordinates
        Payload->Initialize(EmitterTime, -1, RibbonWidth);
    }
}

void UParticleModuleTypeDataRibbon::Update(FParticleContext& Context, float DeltaTime)
{
    // No special update logic needed for multi-particle ribbon
    // Each particle moves independently based on other modules (Velocity, etc.)
    // The ribbon is built from particle positions during rendering
}

void UParticleModuleTypeDataRibbon::BuildRibbonFromParticles(
    FParticleEmitterInstance* EmitterInstance,
    const FVector& EmitterLocation,
    TArray<FVector>& OutPoints,
    TArray<float>& OutWidths,
    TArray<FLinearColor>& OutColors
) const
{
    OutPoints.Empty();
    OutWidths.Empty();
    OutColors.Empty();

    if (!EmitterInstance || EmitterInstance->ActiveParticles <= 0)
        return;

    // Collect all active particles with their data
    TArray<FRibbonParticleData> ParticleDataArray;
    ParticleDataArray.Reserve(EmitterInstance->ActiveParticles);

    int32 ParticleStride = EmitterInstance->ParticleStride;
    uint8* ParticleData = EmitterInstance->ParticleData;

    for (int32 i = 0; i < EmitterInstance->ActiveParticles; ++i)
    {
        // Get particle from memory using stride
        FBaseParticle* Particle = reinterpret_cast<FBaseParticle*>(
            ParticleData + ParticleStride * i
        );

        if (!Particle)
            continue;

        // Get payload
        FRibbonPayload* Payload = GetPayload(Particle);
        if (!Payload)
            continue;

        // Collect particle data for sorting
        // Particle->Location is already in world coordinates
        FRibbonParticleData Data;
        Data.Location = Particle->Location;
        Data.Color = Particle->Color;
        Data.Width = Payload->Width > 0.0f ? Payload->Width : RibbonWidth;
        Data.SpawnTime = Payload->SpawnTime;
        Data.OriginalIndex = i;

        ParticleDataArray.Add(Data);
    }

    // Sort particles by spawn time (oldest first = tail, newest = head)
    std::sort(ParticleDataArray.begin(), ParticleDataArray.end(),
        [](const FRibbonParticleData& A, const FRibbonParticleData& B)
        {
            return A.SpawnTime < B.SpawnTime;
        }
    );

    // Limit to max particles per ribbon
    int32 NumParticles = ParticleDataArray.Num();
    if (NumParticles > MaxParticlesPerRibbon)
    {
        // Remove oldest particles (keep newest)
        int32 ToRemove = NumParticles - MaxParticlesPerRibbon;
        ParticleDataArray.erase(ParticleDataArray.begin(), ParticleDataArray.begin() + ToRemove);
        NumParticles = MaxParticlesPerRibbon;
    }

    if (NumParticles < 2)
        return; // Need at least 2 points to form a ribbon

    // Reserve output arrays
    OutPoints.Reserve(NumParticles);
    OutWidths.Reserve(NumParticles);
    OutColors.Reserve(NumParticles);

    // Build output arrays with taper applied
    for (int32 i = 0; i < NumParticles; ++i)
    {
        const FRibbonParticleData& Data = ParticleDataArray[i];

        // Calculate t (0 = tail/oldest, 1 = head/newest)
        float T = (NumParticles > 1) ?
            static_cast<float>(i) / static_cast<float>(NumParticles - 1) : 1.0f;

        // Calculate width with taper
        float Width = Data.Width;
        if (bTaperRibbon)
        {
            // Taper from TaperFactor at tail to full width at head
            Width = Data.Width * (TaperFactor + (1.0f - TaperFactor) * T);
        }

        // Apply ribbon color tint to particle color
        FLinearColor FinalColor = Data.Color;
        FinalColor.R *= RibbonColor.X;
        FinalColor.G *= RibbonColor.Y;
        FinalColor.B *= RibbonColor.Z;
        FinalColor.A *= RibbonColor.W;

        // Fade alpha at tail
        FinalColor.A *= T;

        OutPoints.Add(Data.Location);
        OutWidths.Add(Width);
        OutColors.Add(FinalColor);
    }
}

FRibbonPayload* UParticleModuleTypeDataRibbon::GetPayload(FBaseParticle* Particle) const
{
    if (!Particle)
        return nullptr;

    // Payload is located after FBaseParticle + PayloadOffset
    uint8* PayloadPtr = reinterpret_cast<uint8*>(Particle)
                      + sizeof(FBaseParticle)
                      + GetPayloadOffset();

    return reinterpret_cast<FRibbonPayload*>(PayloadPtr);
}
