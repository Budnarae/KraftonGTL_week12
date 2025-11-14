#include "pch.h"
#include "AnimationDataModel.h"
#include "FbxLoader.h"

IMPLEMENT_CLASS(UAnimationDataModel)

UAnimationDataModel::UAnimationDataModel()
{
}

UAnimationDataModel::~UAnimationDataModel()
{
}

void UAnimationDataModel::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
    // FBXLoader를 통해 애니메이션 데이터 로드
    // TODO: FBXLoader에 LoadFbxAnimationAsset() 메서드 구현 필요
    
}
