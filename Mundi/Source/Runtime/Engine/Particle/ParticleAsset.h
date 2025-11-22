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
    static UParticleAsset* Create(const FString& InFilePath);
    static void LoadAllDatas();
    UParticleAsset() = default;
    virtual ~UParticleAsset() override = default;

    // ====================================
    // 파일 입출력 (ResourceBase 인터페이스)
    // ====================================

    virtual void Load(const FString& InFilePath, ID3D11Device* InDevice);
    virtual bool Save(const FString& InFilePath = "") override;

    UParticleSystem ParticleSystem;
private:

};
