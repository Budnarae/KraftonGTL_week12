#include "pch.h"
#include "ParticleInstanceBuffer.h"
#include "D3D11RHI.h"

FParticleInstanceBufferManager& FParticleInstanceBufferManager::Get()
{
    static FParticleInstanceBufferManager Instance;
    return Instance;
}

FParticleInstanceBufferManager::~FParticleInstanceBufferManager()
{
    Release();
}

void FParticleInstanceBufferManager::Initialize(int32 InMaxParticleCount)
{
    if (InstanceBuffer)
    {
        // 이미 초기화됨
        return;
    }

    MaxParticleCount = InMaxParticleCount;

    D3D11RHI* RHI = GEngine.GetRHIDevice();
    if (!RHI)
    {
        UE_LOG("[FParticleInstanceBufferManager::Initialize][Error] RHI not available.");
        return;
    }

    // StructuredBuffer 생성
    HRESULT hr = RHI->CreateStructuredBuffer(
        sizeof(FParticleInstanceData),
        MaxParticleCount,
        nullptr,
        &InstanceBuffer
    );

    if (FAILED(hr))
    {
        UE_LOG("[FParticleInstanceBufferManager::Initialize][Error] Failed to create StructuredBuffer.");
        return;
    }

    // SRV 생성
    hr = RHI->CreateStructuredBufferSRV(InstanceBuffer, &InstanceSRV);
    if (FAILED(hr))
    {
        UE_LOG("[FParticleInstanceBufferManager::Initialize][Error] Failed to create SRV.");
        InstanceBuffer->Release();
        InstanceBuffer = nullptr;
        return;
    }
}

void FParticleInstanceBufferManager::Release()
{
    if (InstanceSRV)
    {
        InstanceSRV->Release();
        InstanceSRV = nullptr;
    }

    if (InstanceBuffer)
    {
        InstanceBuffer->Release();
        InstanceBuffer = nullptr;
    }

    MaxParticleCount = 0;
}

ID3D11ShaderResourceView* FParticleInstanceBufferManager::UpdateAndGetSRV(const TArray<FParticleInstanceData>& InstanceData)
{
    if (!InstanceBuffer || !InstanceSRV)
    {
        return nullptr;
    }

    if (InstanceData.empty())
    {
        return nullptr;
    }

    int32 DataCount = static_cast<int32>(InstanceData.size());
    if (DataCount > MaxParticleCount)
    {
        UE_LOG("[FParticleInstanceBufferManager::UpdateAndGetSRV][Warning] Instance count exceeds max. Clamping.");
        DataCount = MaxParticleCount;
    }

    D3D11RHI* RHI = GEngine.GetRHIDevice();
    if (!RHI)
    {
        return nullptr;
    }

    // 버퍼 업데이트
    RHI->UpdateStructuredBuffer(
        InstanceBuffer,
        InstanceData.data(),
        DataCount * sizeof(FParticleInstanceData)
    );

    return InstanceSRV;
}
