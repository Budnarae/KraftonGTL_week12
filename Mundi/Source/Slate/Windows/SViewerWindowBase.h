#pragma once
#include "SWindow.h"

class UWorld;
struct ID3D11Device;

/**
 * @brief 모든 뷰어 윈도우의 베이스 클래스
 * 공통 인터페이스와 기본 동작을 제공합니다.
 */
class SViewerWindowBase : public SWindow
{
public:
    SViewerWindowBase();
    virtual ~SViewerWindowBase();

    /**
     * @brief 뷰어 초기화
     * @param InDevice D3D11 디바이스
     * @param InWorld 월드 포인터
     * @return 초기화 성공 여부
     */
    virtual bool Initialize(ID3D11Device* InDevice, UWorld* InWorld);

    /**
     * @brief 에셋 로드
     * @param AssetPath 로드할 에셋의 경로
     */
    virtual void LoadAsset(const FString& AssetPath) = 0;

    /**
     * @brief 뷰어가 열려있는지 확인
     */
    bool IsOpen() const { return bIsOpen; }

    /**
     * @brief 뷰어 닫기
     */
    virtual void Close() { bIsOpen = false; }

    // SWindow overrides
    virtual void OnRender() override;
    virtual void OnUpdate(float DeltaSeconds) override;
    virtual void OnRenderViewport() {}  // 뷰포트 렌더링 (자식 클래스에서 오버라이드)

protected:
    UWorld* World = nullptr;
    ID3D11Device* Device = nullptr;
    bool bIsOpen = true;
    bool bInitialPlacementDone = false;
    bool bRequestFocus = false;
};
