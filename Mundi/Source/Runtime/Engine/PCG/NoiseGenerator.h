#pragma once

#include "Vector.h"

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

private:
    static float Fade(float T);
    static float Lerp(float A, float B, float T);
    static float Grad(int32 Hash, float X, float Y, float Z);
    static float Grad2D(int32 Hash, float X, float Y);

    static int32 Permutation[512];
    static bool bInitialized;
};
