#pragma once
#include "Object.h"
#include "fbxsdk.h"

class UAnimationSequence;

// FBX 파일에서 추출한 모든 리소스를 담는 구조체
struct FFbxAssetData
{
	FSkeletalMeshData* MeshData = nullptr;
	TArray<UAnimationSequence*> AnimationSequences;
	TArray<FString> AnimationNames; // AnimStack의 실제 이름들
};

class UFbxLoader : public UObject
{
public:

	DECLARE_CLASS(UFbxLoader, UObject)
	static UFbxLoader& GetInstance();
	UFbxLoader();

	static void PreLoad();

	USkeletalMesh* LoadFbxMesh(const FString& FilePath);

	FSkeletalMeshData* LoadFbxMeshAsset(const FString& FilePath);

	FFbxAssetData* LoadFbxAssets(const FString& FilePath);


protected:
	~UFbxLoader() override;
private:
	UFbxLoader(const UFbxLoader&) = delete;
	UFbxLoader& operator=(const UFbxLoader&) = delete;


	void LoadMeshFromNode(FbxNode* InNode, FSkeletalMeshData& MeshData, TMap<int32, TArray<uint32>>& MaterialGroupIndexList, TMap<FbxNode*, int32>& BoneToIndex, TMap<FbxSurfaceMaterial*, int32>& MaterialToIndex);

	void LoadSkeletonFromNode(FbxNode* InNode, FSkeletalMeshData& MeshData, int32 ParentNodeIndex, TMap<FbxNode*, int32>& BoneToIndex);

	void LoadAnimationsFromScene(FbxScene* InScene, const TMap<FbxNode*, int32>& BoneToIndex, const FSkeleton& Skeleton, const FString& FbxPath, TArray<UAnimationSequence*>& OutAnimSequences, TArray<FString>& OutAnimationNames);

	// Scene 생성 및 전처리 (축 변환, 삼각화)
	FbxScene* CreateAndPrepareFbxScene(const FString& FilePath);

	// Scene에서 메시 데이터 추출
	FSkeletalMeshData* ExtractMeshFromScene(FbxScene* InScene, TMap<FbxNode*, int32>& OutBoneToIndex);

	// 캐시 관련 헬퍼
	FSkeletalMeshData* TryLoadMeshFromCache(const FString& FbxPath);
	void SaveMeshToCache(FSkeletalMeshData* MeshData, const FString& FbxPath);
	TArray<UAnimationSequence*> TryLoadAnimationsFromCache(const FString& FbxPath, TArray<FString>& OutAnimationNames);
	
	void LoadMesh(FbxMesh* InMesh, FSkeletalMeshData& MeshData, TMap<int32, TArray<uint32>>& MaterialGroupIndexList, TMap<FbxNode*, int32>& BoneToIndex, TArray<int32> MaterialSlotToIndex, int32 DefaultMaterialIndex = 0);

	void ParseMaterial(FbxSurfaceMaterial* Material, FMaterialInfo& MaterialInfo);

	FString ParseTexturePath(FbxProperty& Property);
	
	void EnsureSingleRootBone(FSkeletalMeshData& MeshData);
	
	// bin파일 저장용
	TArray<FMaterialInfo> MaterialInfos;
	FbxManager* SdkManager = nullptr;
	
};