#include "pch.h"
#include "ResourceBase.h"

IMPLEMENT_CLASS(UResourceBase)

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
