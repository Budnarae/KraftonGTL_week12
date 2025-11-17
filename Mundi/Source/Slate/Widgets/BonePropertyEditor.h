#pragma once

#include <functional>

/**
 * 본 트랜스폼 편집 결과
 */
struct FBonePropertyEditResult
{
    bool bLocationChanged;
    bool bRotationChanged;
    bool bScaleChanged;

    FBonePropertyEditResult()
        : bLocationChanged(false)
        , bRotationChanged(false)
        , bScaleChanged(false)
    {}

    bool AnyChanged() const
    {
        return bLocationChanged || bRotationChanged || bScaleChanged;
    }
};

/**
 * 재사용 가능한 본 프로퍼티 편집 위젯
 *
 * 용도:
 * - 스켈레탈 메시 뷰어에서 본 트랜스폼 편집
 * - 애니메이션 에디터에서 키프레임 편집
 * - 기타 트랜스폼 편집이 필요한 곳
 *
 * 특징:
 * - Location, Rotation, Scale 편집 UI 제공
 * - 색상으로 구분된 직관적인 UI
 * - 변경 감지 및 콜백 지원
 */
class FBonePropertyEditor
{
public:
    FBonePropertyEditor();
    ~FBonePropertyEditor();

    /**
     * 본 프로퍼티 편집 UI를 렌더링합니다.
     *
     * @param BoneName - 표시할 본 이름 (nullptr이면 표시 안 함)
     * @param InOutLocation - 위치 벡터 (in/out)
     * @param InOutRotation - 회전 벡터 (Euler angles in degrees, in/out)
     * @param InOutScale - 스케일 벡터 (in/out)
     * @return 변경 정보 (Location/Rotation/Scale 각각)
     */
    FBonePropertyEditResult Render(
        const char* BoneName,
        FVector& InOutLocation,
        FVector& InOutRotation,  // Euler angles in degrees
        FVector& InOutScale
    );

    /**
     * 본 이름만 표시하고 트랜스폼 편집 UI는 렌더링하지 않습니다.
     */
    void RenderBoneNameOnly(const char* BoneName);

    /**
     * 트랜스폼 변경 시 호출될 콜백을 설정합니다.
     *
     * @param Callback - void() 형태의 콜백 함수
     */
    void SetOnTransformChangedCallback(std::function<void()> Callback);

    /**
     * 편집 중 상태를 가져옵니다 (회전 편집 시 업데이트 방지용)
     */
    bool IsEditing() const { return bIsEditing; }

    /**
     * Location 드래그 속도 설정 (기본값: 0.1f)
     */
    void SetLocationDragSpeed(float Speed) { LocationDragSpeed = Speed; }

    /**
     * Rotation 드래그 속도 설정 (기본값: 0.5f)
     */
    void SetRotationDragSpeed(float Speed) { RotationDragSpeed = Speed; }

    /**
     * Scale 드래그 속도 설정 (기본값: 0.01f)
     */
    void SetScaleDragSpeed(float Speed) { ScaleDragSpeed = Speed; }

private:
    // 변경 콜백
    std::function<void()> OnTransformChanged;

    // 편집 중 상태 (회전 편집 시 외부 업데이트 방지)
    bool bIsEditing;

    // 드래그 속도
    float LocationDragSpeed;
    float RotationDragSpeed;
    float ScaleDragSpeed;
};
