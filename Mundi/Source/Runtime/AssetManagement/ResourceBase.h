#pragma once
#include <d3d11.h>
#include <filesystem>
#include "Object.h"

class UResourceBase : public UObject
{
public:
	DECLARE_CLASS(UResourceBase, UObject)

	UResourceBase() = default;
	virtual ~UResourceBase() {}

	const FString& GetFilePath() const { return FilePath; }
	void SetFilePath(const FString& InFilePath) { FilePath = InFilePath; }

	// Hot Reload Support
	std::filesystem::file_time_type GetLastModifiedTime() const { return LastModifiedTime; }
	void SetLastModifiedTime(std::filesystem::file_time_type InTime) { LastModifiedTime = InTime; }

	/**
	 * @brief 리소스를 파일로 저장합니다.
	 * @param InOutputPath 저장할 경로 (비어있으면 FilePath 사용)
	 * @return 성공 여부
	 */
	virtual bool Save(const FString& InOutputPath = "") { return false; }

	/**
	 * @brief 파일에서 리소스를 로드합니다.
	 * @param InFilePath 로드할 파일 경로
	 * @return 성공 여부
	 */
	virtual bool Load(const FString& InFilePath) { return false; }

	static void RegisterLoadAssetFunc(const FString& ClassName, std::function<UResourceBase* (const FString&)> Func);
	static bool LoadAsset(UResourceBase*& OutAssetPointer, const FString& ClassName, const FString& FilePath);

	static FWideString OpenFileDialogGetPath(const std::filesystem::path& FolderPath, const FWideString& Extension, const FWideString& Desc);
protected:
	FString FilePath;	// 원본 파일의 경로이자, UResourceManager에 등록된 Key
	std::filesystem::file_time_type LastModifiedTime;
private:
	static TMap<FString, std::function<UResourceBase* (const FString&)>>& GetLoadAssetFuncMap();
};