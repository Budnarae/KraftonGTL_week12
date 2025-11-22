#include "pch.h"
#include "NoiseGenerator.h"
#include <cmath>
#include <algorithm>
#include <random>

int32 FNoiseGenerator::Permutation[512] = { 0 };
bool FNoiseGenerator::bInitialized = false;

void FNoiseGenerator::Initialize(int32 Seed)
{
    int32 P[256] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
        8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
        35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
        134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
        55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
        18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
        250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
        189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
        172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
        228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
        107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };

    if (Seed != 0)
    {
        std::mt19937 Rng(Seed);
        std::shuffle(std::begin(P), std::end(P), Rng);
    }

    for (int32 i = 0; i < 256; ++i)
    {
        Permutation[i] = P[i];
        Permutation[256 + i] = P[i];
    }

    bInitialized = true;
}

float FNoiseGenerator::Fade(float T)
{
    return T * T * T * (T * (T * 6.0f - 15.0f) + 10.0f);
}

float FNoiseGenerator::Lerp(float A, float B, float T)
{
    return A + T * (B - A);
}

float FNoiseGenerator::Grad2D(int32 Hash, float X, float Y)
{
    int32 H = Hash & 7;
    float U = H < 4 ? X : Y;
    float V = H < 4 ? Y : X;
    return ((H & 1) ? -U : U) + ((H & 2) ? -2.0f * V : 2.0f * V);
}

float FNoiseGenerator::Grad(int32 Hash, float X, float Y, float Z)
{
    int32 H = Hash & 15;
    float U = H < 8 ? X : Y;
    float V = H < 4 ? Y : (H == 12 || H == 14 ? X : Z);
    return ((H & 1) == 0 ? U : -U) + ((H & 2) == 0 ? V : -V);
}

float FNoiseGenerator::Perlin2D(float X, float Y)
{
    if (!bInitialized)
    {
        Initialize();
    }

    int32 Xi = static_cast<int32>(std::floor(X)) & 255;
    int32 Yi = static_cast<int32>(std::floor(Y)) & 255;

    float Xf = X - std::floor(X);
    float Yf = Y - std::floor(Y);

    float U = Fade(Xf);
    float V = Fade(Yf);

    int32 AA = Permutation[Permutation[Xi] + Yi];
    int32 AB = Permutation[Permutation[Xi] + Yi + 1];
    int32 BA = Permutation[Permutation[Xi + 1] + Yi];
    int32 BB = Permutation[Permutation[Xi + 1] + Yi + 1];

    float X1 = Lerp(Grad2D(AA, Xf, Yf), Grad2D(BA, Xf - 1.0f, Yf), U);
    float X2 = Lerp(Grad2D(AB, Xf, Yf - 1.0f), Grad2D(BB, Xf - 1.0f, Yf - 1.0f), U);

    return Lerp(X1, X2, V);
}

float FNoiseGenerator::Perlin3D(float X, float Y, float Z)
{
    if (!bInitialized)
    {
        Initialize();
    }

    int32 Xi = static_cast<int32>(std::floor(X)) & 255;
    int32 Yi = static_cast<int32>(std::floor(Y)) & 255;
    int32 Zi = static_cast<int32>(std::floor(Z)) & 255;

    float Xf = X - std::floor(X);
    float Yf = Y - std::floor(Y);
    float Zf = Z - std::floor(Z);

    float U = Fade(Xf);
    float V = Fade(Yf);
    float W = Fade(Zf);

    int32 A = Permutation[Xi] + Yi;
    int32 AA = Permutation[A] + Zi;
    int32 AB = Permutation[A + 1] + Zi;
    int32 B = Permutation[Xi + 1] + Yi;
    int32 BA = Permutation[B] + Zi;
    int32 BB = Permutation[B + 1] + Zi;

    float X1 = Lerp(
        Lerp(Grad(Permutation[AA], Xf, Yf, Zf), Grad(Permutation[BA], Xf - 1.0f, Yf, Zf), U),
        Lerp(Grad(Permutation[AB], Xf, Yf - 1.0f, Zf), Grad(Permutation[BB], Xf - 1.0f, Yf - 1.0f, Zf), U),
        V
    );

    float X2 = Lerp(
        Lerp(Grad(Permutation[AA + 1], Xf, Yf, Zf - 1.0f), Grad(Permutation[BA + 1], Xf - 1.0f, Yf, Zf - 1.0f), U),
        Lerp(Grad(Permutation[AB + 1], Xf, Yf - 1.0f, Zf - 1.0f), Grad(Permutation[BB + 1], Xf - 1.0f, Yf - 1.0f, Zf - 1.0f), U),
        V
    );

    return Lerp(X1, X2, W);
}

float FNoiseGenerator::NormalizedPerlin2D(float X, float Y)
{
    return (Perlin2D(X, Y) + 1.0f) * 0.5f;
}

float FNoiseGenerator::NormalizedPerlin3D(float X, float Y, float Z)
{
    return (Perlin3D(X, Y, Z) + 1.0f) * 0.5f;
}

float FNoiseGenerator::FBM2D(float X, float Y, int32 Octaves, float Persistence, float Lacunarity)
{
    float Total = 0.0f;
    float Amplitude = 1.0f;
    float Frequency = 1.0f;
    float MaxValue = 0.0f;

    for (int32 i = 0; i < Octaves; ++i)
    {
        Total += Perlin2D(X * Frequency, Y * Frequency) * Amplitude;
        MaxValue += Amplitude;
        Amplitude *= Persistence;
        Frequency *= Lacunarity;
    }

    return Total / MaxValue;
}

float FNoiseGenerator::FBM3D(float X, float Y, float Z, int32 Octaves, float Persistence, float Lacunarity)
{
    float Total = 0.0f;
    float Amplitude = 1.0f;
    float Frequency = 1.0f;
    float MaxValue = 0.0f;

    for (int32 i = 0; i < Octaves; ++i)
    {
        Total += Perlin3D(X * Frequency, Y * Frequency, Z * Frequency) * Amplitude;
        MaxValue += Amplitude;
        Amplitude *= Persistence;
        Frequency *= Lacunarity;
    }

    return Total / MaxValue;
}
