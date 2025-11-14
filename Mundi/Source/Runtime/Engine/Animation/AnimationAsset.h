#pragma once
#include "ResourceBase.h"

class UAnimationAsset : public UResourceBase
{
    DECLARE_CLASS(UAnimationAsset, UResourceBase)
public:
    UAnimationAsset() = default;
    virtual ~UAnimationAsset() override = default;

    virtual void Load(const FString& InFilePath, ID3D11Device* InDevice) = 0;
    virtual void Save(const FString& InFilePath) = 0;
};
