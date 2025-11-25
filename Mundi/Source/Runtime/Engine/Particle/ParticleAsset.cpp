#include "pch.h"
#include "ParticleAsset.h"
#include "Source/Editor/PlatformProcess.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include <filesystem>

//const FWideString BaseDir = UTF8ToWide(GContentDir) + L"/Particle";
//const FWideString Extension = L".uaaset";
//const FWideString Description = L"Particle Files";
//FWideString DefaultFileName = L"NewParticle";
//
//// Windows 파일 다이얼로그 열기
//std::filesystem::path SelectedPath = FPlatformProcess::OpenSaveFileDialog(BaseDir, Extension, Description, DefaultFileName);
//if (SelectedPath.empty())
//return false;

IMPLEMENT_ASSET_CLASS(UParticleAsset)

void UParticleAsset::LoadAllDatas()
{
    // ========== Phase 1: 캐시 생성 ==========
    UE_LOG("UFbxLoader::Preload - Phase 1: Baking FBX caches...");

    for (const auto& Entry : fs::recursive_directory_iterator(GContentDir))
    {
        if (!Entry.is_regular_file())
            continue;

        const fs::path& Path = Entry.path();
        FString Extension = WideToUTF8(Path.extension().wstring());
        std::transform(Extension.begin(), Extension.end(), Extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (Extension == ".uasset")
        {
            FString PathStr = NormalizePath(WideToUTF8(Path.wstring()));
			UResourceManager::GetInstance().Load<UParticleAsset>(WideToUTF8(Path.wstring()));
        }
    }
}
UParticleAsset* UParticleAsset::Create(const FString& FolderPath)
{
    UParticleAsset* ParticleAsset = NewObject<UParticleAsset>();
	std::filesystem::path TotalPath = FolderPath + "/NewParticle.uasset";
	uint32 idx = 0;
	while (true) 
	{
		UParticleAsset* Asset = UResourceManager::GetInstance().Get<UParticleAsset>(TotalPath.string());
		if (Asset == nullptr)
		{
			break;
		}
		else
		{
			TotalPath = FolderPath + "/NewParticle" + std::to_string(idx++) + ".uasset";
			if (idx == 100)
			{
				return nullptr;
			}
		}
	}

	FString PathStr = TotalPath.string();
	UResourceManager::GetInstance().Add<UParticleAsset>(PathStr, ParticleAsset);
    ParticleAsset->Save();
	return ParticleAsset;
}

void UParticleAsset::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
	JSON OutJson;
	FJsonSerializer::LoadJsonFromFile(OutJson, UTF8ToWide(InFilePath));
	auto temp = &ParticleSystem.Emitters;
	ParticleSystem.Serialize(true, OutJson);
}
bool UParticleAsset::Save(const FString& InFilePath) 
{
	FString PathStr = InFilePath;
	if (InFilePath.empty() == true)
	{
		PathStr = GetFilePath();
	}
	
	JSON Json;
	ParticleSystem.Serialize(false, Json);
	bool bSuccess = FJsonSerializer::SaveJsonToFile(Json, UTF8ToWide(PathStr));
	return bSuccess;
}

