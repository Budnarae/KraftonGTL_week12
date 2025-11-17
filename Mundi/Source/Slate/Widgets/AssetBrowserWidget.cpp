#include "pch.h"
#include "AssetBrowserWidget.h"

#include "PlatformProcess.h"
#include "ThirdParty/imgui/imgui.h"


FAssetBrowserWidget::FAssetBrowserWidget()
    : BrowseButtonLabel("Browse...")
    , LoadButtonLabel("Load FBX")
    , PlaceholderText("Browse for FBX file...")
    , OnLoadAsset(nullptr)
{
    PathBuffer[0] = '\0';
}

FAssetBrowserWidget::~FAssetBrowserWidget()
{
}

bool FAssetBrowserWidget::Render(
    const char* FileExtension,
    const char* FileFilter,
    float ButtonWidth
)
{
    bool bLoadRequested = false;

    ImGui::BeginGroup();

    // Mesh path input
    ImGui::Text("Mesh Path:");
    ImGui::PushItemWidth(-1.0f);
    ImGui::InputTextWithHint("##AssetPath", PlaceholderText, PathBuffer, sizeof(PathBuffer));
    ImGui::PopItemWidth();

    ImGui::Spacing();

    // 버튼 너비 계산 (자동)
    float actualButtonWidth = ButtonWidth;
    if (actualButtonWidth < 0.0f)
    {
        float availableWidth = ImGui::GetContentRegionAvail().x;
        actualButtonWidth = (availableWidth - 8.0f) * 0.5f;  // 2개 버튼, 8px 간격
    }

    // Browse 버튼
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.40f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.50f, 0.70f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.35f, 0.50f, 1.0f));

    if (ImGui::Button(BrowseButtonLabel, ImVec2(actualButtonWidth, 32)))
    {
        // 파일 다이얼로그 열기
        std::wstring wideExt = UTF8ToWide(FileExtension);
        std::wstring wideFilter = UTF8ToWide(FileFilter);

        auto widePath = FPlatformProcess::OpenLoadFileDialog(UTF8ToWide(GDataDir), wideExt.c_str(), wideFilter.c_str());
        if (!widePath.empty())
        {
            std::string pathStr = widePath.string();
            strncpy_s(PathBuffer, pathStr.c_str(), sizeof(PathBuffer) - 1);
        }
    }

    ImGui::SameLine();

    // Load 버튼 (다른 색상)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.60f, 0.40f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.70f, 0.50f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.50f, 0.30f, 1.0f));

    if (ImGui::Button(LoadButtonLabel, ImVec2(actualButtonWidth, 32)))
    {
        FString Path = PathBuffer;
        if (!Path.empty())
        {
            bLoadRequested = true;

            // 콜백 호출
            if (OnLoadAsset)
            {
                OnLoadAsset(Path);
            }
        }
    }

    ImGui::PopStyleColor(6);
    ImGui::EndGroup();

    return bLoadRequested;
}

void FAssetBrowserWidget::SetOnLoadAssetCallback(std::function<void(const FString&)> Callback)
{
    OnLoadAsset = Callback;
}

const char* FAssetBrowserWidget::GetPath() const
{
    return PathBuffer;
}

void FAssetBrowserWidget::SetPath(const FString& Path)
{
    strncpy_s(PathBuffer, Path.c_str(), sizeof(PathBuffer) - 1);
}

void FAssetBrowserWidget::ClearPath()
{
    PathBuffer[0] = '\0';
}

void FAssetBrowserWidget::SetBrowseButtonLabel(const char* Label)
{
    BrowseButtonLabel = Label;
}

void FAssetBrowserWidget::SetLoadButtonLabel(const char* Label)
{
    LoadButtonLabel = Label;
}

void FAssetBrowserWidget::SetPlaceholderText(const char* Text)
{
    PlaceholderText = Text;
}
