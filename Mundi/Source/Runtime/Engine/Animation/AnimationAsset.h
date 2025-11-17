#pragma once
#include "ResourceBase.h"
#include "AnimationType.h"

/**
 * @brief 모든 애니메이션 에셋의 추상 베이스 클래스
 * @details AnimSequence, AnimMontage, BlendSpace 등이 상속받습니다
 */
class UAnimationAsset : public UResourceBase
{
    DECLARE_CLASS(UAnimationAsset, UResourceBase)
public:
    UAnimationAsset() = default;
    virtual ~UAnimationAsset() override = default;

    // ====================================
    // 파일 입출력 (ResourceBase 인터페이스)
    // ====================================

    virtual void Load(const FString& InFilePath, ID3D11Device* InDevice) = 0;
    virtual bool Save(const FString& InFilePath = "") override = 0;

    // ====================================
    // 애니메이션 재생 인터페이스
    // ====================================

    /**
     * @brief 특정 본의 특정 시간에서의 포즈를 반환합니다
     * @param BoneName 본 이름
     * @param Time 애니메이션 시간 (초)
     * @return 해당 시간의 본 트랜스폼 (LocalSpace)
     */
    virtual FTransform GetBonePose(const FName& BoneName, float Time) const = 0;

    /**
     * @brief 애니메이션의 총 재생 길이를 반환합니다
     * @return 재생 길이 (초)
     */
    virtual float GetPlayLength() const = 0;

    /**
     * @brief 애니메이션의 프레임레이트를 반환합니다
     * @return 프레임레이트 (fps)
     */
    virtual float GetFrameRate() const = 0;

    // Anim Node에서 Update, Evaluate이 쓰이지만,
    // AnimSinglenodeInstance같은 단일 Sequence만 재생하는 상황에 별도의 AnimNode를 만들지 않고 재생하는 설계의 유연성을 위해 남겨둠
    // Anim Node의 Update, Evaluate에서도 이 Update, Evaluate를 부름
    // 추후 Anim Node에 이 로직을 전부 흡수해도 됨. 판단은 알아서.
    virtual void Update(const FAnimationUpdateContext& Context) = 0;
    virtual void Evaluate(FPoseContext& Output) = 0;
};
