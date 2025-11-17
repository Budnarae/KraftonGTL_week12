#pragma once
#include "SWindow.h"

class UWorld;
struct ID3D11Device;

/**
 * @brief 뷰어 탭 상태 베이스 클래스
 * 각 뷰어는 이를 상속받아 자신만의 탭 상태를 구현합니다.
 */
struct ViewerTabStateBase
{
    FString Name;

    ViewerTabStateBase(const FString& InName = "Tab") : Name(InName) {}
    virtual ~ViewerTabStateBase() = default;
};

/**
 * @brief 모든 뷰어 윈도우의 베이스 클래스
 * 공통 인터페이스와 기본 동작을 제공합니다.
 * 탭 기반 뷰어 시스템을 지원합니다.
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
    // === 탭 관리 시스템 ===

    /**
     * @brief 새 탭 생성 (자식 클래스에서 구현)
     * @param Name 탭 이름
     * @return 생성된 탭 상태 포인터
     */
    virtual ViewerTabStateBase* CreateTabState(const char* Name) = 0;

    /**
     * @brief 탭 상태 삭제 (자식 클래스에서 구현)
     * @param State 삭제할 탭 상태
     */
    virtual void DestroyTabState(ViewerTabStateBase* State) = 0;

    /**
     * @brief 탭 바 렌더링 (공통 로직)
     */
    void RenderTabBar();

    /**
     * @brief 새 탭 추가
     * @param Name 탭 이름
     */
    void OpenNewTab(const char* Name);

    /**
     * @brief 탭 닫기
     * @param Index 닫을 탭 인덱스
     */
    void CloseTab(int Index);

    /**
     * @brief 활성 탭 상태 가져오기
     */
    ViewerTabStateBase* GetActiveTabState() const { return ActiveState; }

    /**
     * @brief 활성 탭 인덱스 가져오기
     */
    int GetActiveTabIndex() const { return ActiveTabIndex; }

protected:
    UWorld* World = nullptr;
    ID3D11Device* Device = nullptr;
    bool bIsOpen = true;
    bool bInitialPlacementDone = false;
    bool bRequestFocus = false;

    // 탭 관리
    TArray<ViewerTabStateBase*> Tabs;
    ViewerTabStateBase* ActiveState = nullptr;
    int ActiveTabIndex = -1;
};
