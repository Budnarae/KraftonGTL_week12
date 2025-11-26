#pragma once
#include "ResourceBase.h"
#include "Source/Runtime/Engine/Particle/ParticleSystem.h"

/**
 * @brief 파티클 시스템 에셋 클래스
 * @details 파티클 시스템 데이터를 파일로 저장/로드합니다
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

    // ParticleSystem 접근자
    UParticleSystem* GetParticleSystem() { return &ParticleSystem; }
    const UParticleSystem* GetParticleSystem() const { return &ParticleSystem; }

    UParticleSystem ParticleSystem;
};
