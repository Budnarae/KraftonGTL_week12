#include "pch.h"
#include "ResourceBase.h"
#include "Source/Editor/PlatformProcess.h"

IMPLEMENT_CLASS(UResourceBase)
fs::path GetPathFromContent(const fs::path& fullPath)
{
    fs::path result;
    bool found = false;

    for (const auto& part : fullPath)
    {
        if (!found && part == "Content")
            found = true;
        if (found)
            result /= part;
    }

    return result; // Content 폴더부터 끝까지
}

TMap<FString, std::function<UResourceBase* (const FString&)>>& UResourceBase::GetLoadAssetFuncMap()
{
	static TMap<FString, std::function<UResourceBase* (const FString&)>> Map;
	return Map;
}

void UResourceBase::RegisterLoadAssetFunc(const FString& ClassName, std::function<UResourceBase* (const FString&)> Func)
{
	GetLoadAssetFuncMap()[ClassName] = Func;
}

bool UResourceBase::LoadAsset(UResourceBase*& OutAssetPointer, const FString& ClassName, const FString& FilePath)
{
	if (GetLoadAssetFuncMap().Contains(ClassName))
	{
		OutAssetPointer = GetLoadAssetFuncMap()[ClassName](FilePath);
		if (OutAssetPointer != nullptr)
		{
			return true;
		}
	}
	return false;
}


FWideString UResourceBase::OpenFileDialogGetPath(const std::filesystem::path& FolderPath, const FWideString& Extension, const FWideString& Desc)
{
    // Windows 파일 다이얼로그 열기
    std::filesystem::path SelectedPath = FPlatformProcess::OpenLoadFileDialog(FolderPath.wstring(), Extension, Desc);
    SelectedPath = GetPathFromContent(SelectedPath);
    if (SelectedPath.empty())
    {
        return L"";
    }
    else
    {
        return SelectedPath.wstring();
    }
}