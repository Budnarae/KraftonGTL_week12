#pragma once
#include <functional>

/**
 * 재사용 가능한 에셋 브라우저 위젯
 *
 * 용도:
 * - 스켈레탈 메시 뷰어에서 FBX 파일 로드
 * - 스태틱 메시 뷰어에서 FBX 파일 로드
 * - 머티리얼 에디터에서 텍스처 로드
 * - 기타 파일 다이얼로그가 필요한 곳
 *
 * 특징:
 * - 파일 경로 입력 필드
 * - Browse 버튼 (파일 다이얼로그)
 * - Load 버튼 (로드 실행)
 * - 확장자 필터링 지원
 */
class FAssetBrowserWidget
{
public:
    FAssetBrowserWidget();
    ~FAssetBrowserWidget();

    /**
     * 에셋 브라우저 UI를 렌더링합니다.
     *
     * @param FileExtension - 파일 확장자 (예: "fbx", "png", "mat")
     * @param FileFilter - 파일 필터 설명 (예: "FBX Files", "Image Files")
     * @param ButtonWidth - Browse/Load 버튼 너비 (-1이면 자동 계산)
     * @return Load 버튼이 클릭되고 유효한 경로가 있으면 true
     */
    bool Render(
        const char* FileExtension = "fbx",
        const char* FileFilter = "FBX Files",
        float ButtonWidth = -1.0f
    );

    /**
     * 로드 콜백을 설정합니다.
     *
     * @param Callback - void(const FString& AssetPath) 형태의 콜백 함수
     */
    void SetOnLoadAssetCallback(std::function<void(const FString&)> Callback);

    /**
     * 현재 경로를 가져옵니다.
     */
    const char* GetPath() const;

    /**
     * 경로를 설정합니다.
     */
    void SetPath(const FString& Path);

    /**
     * 경로를 초기화합니다.
     */
    void ClearPath();

    /**
     * Browse 버튼 레이블 설정 (기본값: "Browse...")
     */
    void SetBrowseButtonLabel(const char* Label);

    /**
     * Load 버튼 레이블 설정 (기본값: "Load FBX")
     */
    void SetLoadButtonLabel(const char* Label);

    /**
     * Placeholder 텍스트 설정 (기본값: "Browse for FBX file...")
     */
    void SetPlaceholderText(const char* Text);

private:
    // 경로 버퍼
    char PathBuffer[260];

    // 버튼 레이블
    const char* BrowseButtonLabel;
    const char* LoadButtonLabel;
    const char* PlaceholderText;

    // 로드 콜백
    std::function<void(const FString&)> OnLoadAsset;
};
