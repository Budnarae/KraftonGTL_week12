#pragma once

#include <functional>

class UAnimNotify;

/**
 * AnimNotify 프로퍼티 편집 결과
 */
struct FNotifyPropertyEditResult
{
	bool bNameChanged = false;
	bool bTimeChanged = false;
	bool bPropertiesChanged = false;
	bool bDeleted = false;

	bool AnyChanged() const
	{
		return bNameChanged || bTimeChanged || bPropertiesChanged;
	}
};

/**
 * 재사용 가능한 AnimNotify 프로퍼티 편집 위젯
 *
 * 용도:
 * - 애니메이션 뷰어에서 notify 프로퍼티 편집
 * - 타입별 분기 처리 (SoundNotify, CameraShakeNotify 등)
 *
 * 특징:
 * - 공통 프로퍼티 (Name, Time) 편집
 * - 타입별 프로퍼티 (Volume, Pitch, Duration 등) 편집
 * - Delete 버튼 제공
 * - 변경 감지 및 콜백 지원
 */
class FNotifyPropertyEditor
{
public:
	FNotifyPropertyEditor();
	~FNotifyPropertyEditor();

	/**
	 * Notify 프로퍼티 편집 UI를 렌더링합니다.
	 *
	 * @param InOutNotify - 편집할 notify (in/out, Delete 시 nullptr로 설정됨)
	 * @param MaxTime - 최대 시간 (애니메이션 길이)
	 * @return 변경 정보
	 */
	FNotifyPropertyEditResult Render(UAnimNotify*& InOutNotify, float MaxTime);

	/**
	 * Notify가 선택되지 않았을 때 표시할 메시지를 렌더링합니다.
	 */
	void RenderEmptyState();

	/**
	 * Delete 버튼 클릭 시 호출될 콜백을 설정합니다.
	 *
	 * @param Callback - void(UAnimNotify*) 형태의 콜백 함수
	 */
	void SetOnDeleteCallback(std::function<void(UAnimNotify*)> Callback);

private:
	/**
	 * 공통 프로퍼티 (Name, Time)를 렌더링합니다.
	 */
	bool RenderCommonProperties(UAnimNotify* Notify, float MaxTime);

	/**
	 * SoundAnimNotify 프로퍼티를 렌더링합니다.
	 */
	bool RenderSoundNotifyProperties(UAnimNotify* Notify);

	/**
	 * CameraShakeAnimNotify 프로퍼티를 렌더링합니다.
	 */
	bool RenderCameraShakeNotifyProperties(UAnimNotify* Notify);

private:
	// Delete 콜백
	std::function<void(UAnimNotify*)> OnDelete;
};
