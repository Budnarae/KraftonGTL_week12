#include "pch.h"
#include "SkeletalMesh.h"


#include "FbxLoader.h"
#include "WindowsBinReader.h"
#include "WindowsBinWriter.h"
#include "PathUtils.h"
#include <filesystem>

IMPLEMENT_CLASS(USkeletalMesh)

USkeletalMesh::USkeletalMesh()
{
}

USkeletalMesh::~USkeletalMesh()
{
    ReleaseResources();
}

void USkeletalMesh::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
    if (Data)
    {
        ReleaseResources();
    }

    // FBXLoader가 캐싱을 내부적으로 처리합니다
    Data = UFbxLoader::GetInstance().LoadFbxMeshAsset(InFilePath);

    if (!Data || Data->Vertices.empty() || Data->Indices.empty())
    {
        UE_LOG("ERROR: Failed to load FBX mesh from '%s'", InFilePath.c_str());
        return;
    }

    // GPU 버퍼 생성
    CreateIndexBuffer(Data, InDevice);
    VertexCount = static_cast<uint32>(Data->Vertices.size());
    IndexCount = static_cast<uint32>(Data->Indices.size());
    VertexStride = sizeof(FVertexDynamic);
}

bool USkeletalMesh::Save(const FString& InOutputPath)
{
    if (!Data)
    {
        UE_LOG("SkeletalMesh::Save failed: No mesh data to save");
        return false;
    }

    FString SavePath = InOutputPath.empty() ? FilePath : InOutputPath;
    if (SavePath.empty())
    {
        UE_LOG("SkeletalMesh::Save failed: No file path specified");
        return false;
    }

    // 디렉토리 생성
    std::filesystem::path FilePathObj(SavePath);
    if (FilePathObj.has_parent_path())
    {
        std::filesystem::create_directories(FilePathObj.parent_path());
    }

    try
    {
        FWindowsBinWriter Writer(SavePath);
        Writer << *Data;
        Writer.Close();

        UE_LOG("SkeletalMesh saved: %s (%d vertices, %d indices)",
               SavePath.c_str(), Data->Vertices.Num(), Data->Indices.Num());
        return true;
    }
    catch (const std::exception& e)
    {
        UE_LOG("SkeletalMesh::Save failed: %s", e.what());
        return false;
    }
}

void USkeletalMesh::InitFromData(FSkeletalMeshData* InData, ID3D11Device* InDevice)
{
    if (Data)
    {
        ReleaseResources();
    }

    Data = InData;

    if (!Data || Data->Vertices.empty() || Data->Indices.empty())
    {
        UE_LOG("ERROR: Invalid SkeletalMeshData provided to InitFromData");
        return;
    }

    // GPU 버퍼 생성
    CreateIndexBuffer(Data, InDevice);
    VertexCount = static_cast<uint32>(Data->Vertices.size());
    IndexCount = static_cast<uint32>(Data->Indices.size());
    VertexStride = sizeof(FVertexDynamic);
}

void USkeletalMesh::ReleaseResources()
{
    if (IndexBuffer)
    {
        IndexBuffer->Release();
        IndexBuffer = nullptr;
    }

    if (Data)
    {
        delete Data;
        Data = nullptr;
    }
}

void USkeletalMesh::CreateVertexBuffer(ID3D11Buffer** InVertexBuffer, bool bUseSkinningAttributes)
{
    if (!Data) { return; }
    ID3D11Device* Device = GEngine.GetRHIDevice()->GetDevice();

    HRESULT hr = E_FAIL;
    if (bUseSkinningAttributes)
    {
        hr = D3D11RHI::CreateVertexBuffer<FSkinnedVertex>(Device, Data->Vertices, InVertexBuffer);
    }
    else
    {
        hr = D3D11RHI::CreateVertexBuffer<FVertexDynamic>(Device, Data->Vertices, InVertexBuffer);
    }
    assert(SUCCEEDED(hr));
}

void USkeletalMesh::UpdateVertexBuffer(const TArray<FNormalVertex>& SkinnedVertices, ID3D11Buffer* InVertexBuffer)
{
    if (!InVertexBuffer) { return; }

    GEngine.GetRHIDevice()->VertexBufferUpdate(InVertexBuffer, SkinnedVertices);
}

void USkeletalMesh::CreateIndexBuffer(FSkeletalMeshData* InSkeletalMesh, ID3D11Device* InDevice)
{
    HRESULT hr = D3D11RHI::CreateIndexBuffer(InDevice, InSkeletalMesh, &IndexBuffer);
    assert(SUCCEEDED(hr));
}
