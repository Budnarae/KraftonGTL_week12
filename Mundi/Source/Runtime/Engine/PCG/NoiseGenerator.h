#pragma once

#include "Vector.h"
#include "UEContainer.h"

class FNoiseGenerator
{
public:
    static void Initialize(int32 Seed = 0);

    // Perlin Noise
    static float Perlin2D(float X, float Y);
    static float Perlin3D(float X, float Y, float Z);

    // Fractal Brownian Motion
    static float FBM2D(float X, float Y, int32 Octaves = 4, float Persistence = 0.5f, float Lacunarity = 2.0f);
    static float FBM3D(float X, float Y, float Z, int32 Octaves = 4, float Persistence = 0.5f, float Lacunarity = 2.0f);

    // 0~1 normalized noise
    static float NormalizedPerlin2D(float X, float Y);
    static float NormalizedPerlin3D(float X, float Y, float Z);

    // Midpoint Displacement (번개 생성용)
    // Start에서 End까지 Midpoint Displacement 알고리즘으로 포인트 생성
    // Depth: 재귀 깊이 (높을수록 더 많은 꺾임)
    // Displacement: 초기 변위량
    // TimeSeed: 시간 기반 시드 (동적 노이즈용)
    // DisplacementDecay: 각 재귀 레벨에서 변위량이 감소하는 비율 (0.0 ~ 1.0)
    static void GenerateLightningPoints(
        const FVector& Start,
        const FVector& End,
        int32 Depth,
        float Displacement,
        int32 TimeSeed,
        TArray<FVector>& OutPoints,
        float DisplacementDecay = 0.5f
    );

    // 해시 기반 랜덤 (-1 ~ 1)
    static float HashRandom(int32 Seed);

private:
    static float Fade(float T);
    static float Lerp(float A, float B, float T);
    static float Grad(int32 Hash, float X, float Y, float Z);
    static float Grad2D(int32 Hash, float X, float Y);

    static int32 Permutation[512];
    static bool bInitialized;
};
