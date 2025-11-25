#include "pch.h"
#include "DynamicMeshBuffer.h"
#include "D3D11RHI.h"

FDynamicMeshBuffer::~FDynamicMeshBuffer()
{
    Release();
}

FDynamicMeshBuffer::FDynamicMeshBuffer(FDynamicMeshBuffer&& Other) noexcept
    : VertexBuffer(Other.VertexBuffer)
    , IndexBuffer(Other.IndexBuffer)
    , VertexCapacity(Other.VertexCapacity)
    , IndexCapacity(Other.IndexCapacity)
    , VertexCount(Other.VertexCount)
    , IndexCount(Other.IndexCount)
{
    Other.VertexBuffer = nullptr;
    Other.IndexBuffer = nullptr;
    Other.VertexCapacity = 0;
    Other.IndexCapacity = 0;
    Other.VertexCount = 0;
    Other.IndexCount = 0;
}

FDynamicMeshBuffer& FDynamicMeshBuffer::operator=(FDynamicMeshBuffer&& Other) noexcept
{
    if (this != &Other)
    {
        Release();

        VertexBuffer = Other.VertexBuffer;
        IndexBuffer = Other.IndexBuffer;
        VertexCapacity = Other.VertexCapacity;
        IndexCapacity = Other.IndexCapacity;
        VertexCount = Other.VertexCount;
        IndexCount = Other.IndexCount;

        Other.VertexBuffer = nullptr;
        Other.IndexBuffer = nullptr;
        Other.VertexCapacity = 0;
        Other.IndexCapacity = 0;
        Other.VertexCount = 0;
        Other.IndexCount = 0;
    }
    return *this;
}

bool FDynamicMeshBuffer::UpdateVertices(const TArray<FBillboardVertex>& Vertices)
{
    if (Vertices.empty())
    {
        VertexCount = 0;
        return true;
    }

    uint32 RequiredCount = static_cast<uint32>(Vertices.size());
    if (!EnsureVertexCapacity(RequiredCount))
    {
        return false;
    }

    D3D11RHI* RHIDevice = GEngine.GetRHIDevice();
    if (!RHIDevice)
    {
        return false;
    }

    ID3D11DeviceContext* Context = RHIDevice->GetDeviceContext();
    if (!Context)
    {
        return false;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    HRESULT hr = Context->Map(VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    if (FAILED(hr))
    {
        UE_LOG("[FDynamicMeshBuffer::UpdateVertices] Failed to map vertex buffer.");
        return false;
    }

    memcpy(MappedResource.pData, Vertices.GetData(), RequiredCount * sizeof(FBillboardVertex));
    Context->Unmap(VertexBuffer, 0);

    VertexCount = RequiredCount;
    return true;
}

bool FDynamicMeshBuffer::UpdateIndices(const TArray<uint32>& Indices)
{
    if (Indices.empty())
    {
        IndexCount = 0;
        return true;
    }

    uint32 RequiredCount = static_cast<uint32>(Indices.size());
    if (!EnsureIndexCapacity(RequiredCount))
    {
        return false;
    }

    D3D11RHI* RHIDevice = GEngine.GetRHIDevice();
    if (!RHIDevice)
    {
        return false;
    }

    ID3D11DeviceContext* Context = RHIDevice->GetDeviceContext();
    if (!Context)
    {
        return false;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    HRESULT hr = Context->Map(IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    if (FAILED(hr))
    {
        UE_LOG("[FDynamicMeshBuffer::UpdateIndices] Failed to map index buffer.");
        return false;
    }

    memcpy(MappedResource.pData, Indices.GetData(), RequiredCount * sizeof(uint32));
    Context->Unmap(IndexBuffer, 0);

    IndexCount = RequiredCount;
    return true;
}

bool FDynamicMeshBuffer::Update(const TArray<FBillboardVertex>& Vertices, const TArray<uint32>& Indices)
{
    return UpdateVertices(Vertices) && UpdateIndices(Indices);
}

void FDynamicMeshBuffer::Release()
{
    if (VertexBuffer)
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }
    if (IndexBuffer)
    {
        IndexBuffer->Release();
        IndexBuffer = nullptr;
    }
    VertexCapacity = 0;
    IndexCapacity = 0;
    VertexCount = 0;
    IndexCount = 0;
}

bool FDynamicMeshBuffer::EnsureVertexCapacity(uint32 RequiredCount)
{
    if (VertexBuffer && VertexCapacity >= RequiredCount)
    {
        return true;
    }

    D3D11RHI* RHIDevice = GEngine.GetRHIDevice();
    if (!RHIDevice)
    {
        return false;
    }

    ID3D11Device* Device = RHIDevice->GetDevice();
    if (!Device)
    {
        return false;
    }

    // 기존 버퍼 해제
    if (VertexBuffer)
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }

    // 여유 있게 할당 (50% 여유)
    uint32 NewCapacity = RequiredCount + RequiredCount / 2;

    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth = NewCapacity * sizeof(FBillboardVertex);
    BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = Device->CreateBuffer(&BufferDesc, nullptr, &VertexBuffer);
    if (FAILED(hr))
    {
        UE_LOG("[FDynamicMeshBuffer::EnsureVertexCapacity] Failed to create vertex buffer.");
        VertexCapacity = 0;
        return false;
    }

    VertexCapacity = NewCapacity;
    return true;
}

bool FDynamicMeshBuffer::EnsureIndexCapacity(uint32 RequiredCount)
{
    if (IndexBuffer && IndexCapacity >= RequiredCount)
    {
        return true;
    }

    D3D11RHI* RHIDevice = GEngine.GetRHIDevice();
    if (!RHIDevice)
    {
        return false;
    }

    ID3D11Device* Device = RHIDevice->GetDevice();
    if (!Device)
    {
        return false;
    }

    // 기존 버퍼 해제
    if (IndexBuffer)
    {
        IndexBuffer->Release();
        IndexBuffer = nullptr;
    }

    // 여유 있게 할당 (50% 여유)
    uint32 NewCapacity = RequiredCount + RequiredCount / 2;

    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth = NewCapacity * sizeof(uint32);
    BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = Device->CreateBuffer(&BufferDesc, nullptr, &IndexBuffer);
    if (FAILED(hr))
    {
        UE_LOG("[FDynamicMeshBuffer::EnsureIndexCapacity] Failed to create index buffer.");
        IndexCapacity = 0;
        return false;
    }

    IndexCapacity = NewCapacity;
    return true;
}
