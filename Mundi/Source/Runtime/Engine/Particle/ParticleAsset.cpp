#include "pch.h"
#include "ParticleAsset.h"
#include "Source/Editor/PlatformProcess.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include <filesystem>

const std::filesystem::path UParticleAsset::FolderPath = GDataDir + "/Particle";
const FWideString UParticleAsset::Extension = L".uasset";
const FWideString UParticleAsset::Desc = L"Particle Files";
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

/// <summary>
/// 현재 폴더에 NewParticle 이름 안겹치게 생성
/// </summary>
/// <param name="FolderPath"></param>
/// <returns></returns>
UParticleAsset* UParticleAsset::CreateAutoName(const FString& FolderPath)
{
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
	return Create(PathStr);
}
UParticleAsset* UParticleAsset::Create(const FString& InFilePath)
{
	UParticleAsset* ParticleAsset = NewObject<UParticleAsset>();
	UResourceManager::GetInstance().Add<UParticleAsset>(InFilePath, ParticleAsset);
	ParticleAsset->Save();
	return ParticleAsset;
}


void UParticleAsset::Save(const FString& InFilePath, UParticleSystem* InParticle)
{
	if (InParticle == nullptr)
	{
		return;
	}

	//Json 저장
	JSON Json;
	InParticle->Serialize(false, Json);
	bool bSuccess = FJsonSerializer::SaveJsonToFile(Json, UTF8ToWide(InFilePath));

	//Json 파일 강제로 다시 로드해오기
	UResourceManager::GetInstance().ForceLoad<UParticleAsset>(InFilePath);
}

void UParticleAsset::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
	JSON OutJson;
	FJsonSerializer::LoadJsonFromFile(OutJson, UTF8ToWide(InFilePath));
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

//복사본을 획득하는 함수
//기존 Duplication은 얕은복사고 깊은복사에필요
//깊은복사 다 하는건 시간이 없으니 파일을 다시 읽는걸로 하자
void UParticleAsset::GetDeepDuplicated(UParticleSystem* OutParticleSystem) const
{
	JSON OutJson;
	FJsonSerializer::LoadJsonFromFile(OutJson, UTF8ToWide(GetFilePath()));
	OutParticleSystem->Serialize(true, OutJson);
}
