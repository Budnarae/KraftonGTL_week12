#pragma once
#include "ResourceBase.h"
#include "Source/Runtime/Engine/Particle/ParticleSystem.h"

/**
 * @brief 모든 애니메이션 에셋의 추상 베이스 클래스
 * @details AnimSequence, AnimMontage, BlendSpace 등이 상속받습니다
 */       
class UParticleAsset : public UResourceBase
{
    DECLARE_CLASS(UParticleAsset, UResourceBase)
public:
    const static std::filesystem::path FolderPath;
    const static FWideString Extension;
    const static FWideString Desc;
    static void LoadAllDatas();
    static UParticleAsset* CreateAutoName(const FString& InFolderPath);
    static UParticleAsset* Create(const FString& InFilePath);
    static void Save(const FString& InFilePath, UParticleSystem* InParticle);
    UParticleAsset() = default;
    virtual ~UParticleAsset() = default;

    // ====================================
    // 파일 입출력 (ResourceBase 인터페이스)
    // ====================================

    virtual void Load(const FString& InFilePath, ID3D11Device* InDevice);
    virtual bool Save(const FString& InFilePath = "") override;
    void GetDeepDuplicated(UParticleSystem* OutParticleSystem) const;

    UParticleSystem ParticleSystem;
private:

};
