#include "pch.h"
#include "SMaterialEditorWindow.h"
#include "Source/Runtime/Engine/MaterialEditor/MaterialEditorBootstrap.h"
#include "Source/Runtime/Engine/GameFramework/StaticMeshActor.h"
#include "Source/Runtime/Engine/Components/StaticMeshComponent.h"
#include "Source/Runtime/Renderer/Material.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "ThirdParty/imgui/imgui.h"

SMaterialEditorWindow::SMaterialEditorWindow()
    : SViewerWindowBase()
{
    CenterRect = FRect(0, 0, 0, 0);
}

SMaterialEditorWindow::~SMaterialEditorWindow()
{
    if (State)
    {
        MaterialEditorBootstrap::DestroyEditorState(State);
        State = nullptr;
    }
}

bool SMaterialEditorWindow::Initialize(ID3D11Device* InDevice, UWorld* InWorld)
{
    // 베이스 클래스 초기화
    if (!SViewerWindowBase::Initialize(InDevice, InWorld))
        return false;

    // 기본 크기 및 위치 설정
    const float DefaultWidth = 1200.0f;
    const float DefaultHeight = 800.0f;
    const float StartX = 200.0f;
    const float StartY = 100.0f;

    SetRect(StartX, StartY, StartX + DefaultWidth, StartY + DefaultHeight);

    // Create editor state
    State = MaterialEditorBootstrap::CreateEditorState("Editor", World, Device);
    if (State && State->Viewport)
    {
        State->Viewport->Resize((uint32)StartX, (uint32)StartY, (uint32)DefaultWidth, (uint32)DefaultHeight);
    }

    bRequestFocus = true;
    return true;
}

void SMaterialEditorWindow::LoadAsset(const FString& AssetPath)
{
    LoadMaterial(AssetPath);
}

void SMaterialEditorWindow::LoadMaterial(const FString& Path)
{
    if (!State)
        return;

    // TODO: Load material asset
    // UMaterial* Mat = UResourceManager::GetInstance().Load<UMaterial>(Path);
    // State->BaseMaterial = Mat;
    // State->MaterialInstance = UMaterialInstanceDynamic::Create(Mat);
    // Apply to preview actor
}

void SMaterialEditorWindow::OnRender()
{
    if (!State)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    bool bViewerVisible = ImGui::Begin("Material Editor", &bIsOpen, window_flags);

    if (!bViewerVisible)
    {
        ImGui::End();
        CenterRect = FRect(0, 0, 0, 0);
        CenterRect.UpdateMinMax();
        return;
    }

    // 메인 패널 레이아웃
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImVec2 availableRegion = ImGui::GetContentRegionAvail();
    float totalWidth = availableRegion.x;
    float totalHeight = availableRegion.y;

    float leftWidth = totalWidth * LeftPanelRatio;
    float rightWidth = totalWidth * RightPanelRatio;
    float centerWidth = totalWidth - leftWidth - rightWidth;

    // === Left Panel: Material Browser ===
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, totalHeight), true);
    ImGui::PopStyleVar();

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    ImGui::Indent(8.0f);
    ImGui::Text("Material Browser");
    ImGui::Unindent(8.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    float buttonWidth = (leftWidth - 24.0f) * 0.5f - 4.0f;
    AssetBrowser.SetLoadButtonLabel("Load Material");
    AssetBrowser.SetPlaceholderText("Browse for material file...");

    if (AssetBrowser.Render("mat", "Material Files", buttonWidth))
    {
        LoadMaterial(AssetBrowser.GetPath());
    }

    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    // === Center Panel: Viewport ===
    ImGui::BeginChild("MaterialViewport", ImVec2(centerWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImVec2 childPos = ImGui::GetWindowPos();
    ImVec2 childSize = ImGui::GetWindowSize();
    CenterRect.Left = childPos.x;
    CenterRect.Top = childPos.y;
    CenterRect.Right = childPos.x + childSize.x;
    CenterRect.Bottom = childPos.y + childSize.y;
    CenterRect.UpdateMinMax();
    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    // === Right Panel: Parameters ===
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild("RightPanel", ImVec2(rightWidth, totalHeight), true);
    ImGui::PopStyleVar();

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    ImGui::Indent(8.0f);
    ImGui::Text("Material Parameters");
    ImGui::Unindent(8.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Preview Mesh Selection
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.90f, 0.40f, 1.0f));
    ImGui::Text("Preview Mesh");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    const char* meshTypes[] = { "Sphere", "Cube", "Cylinder", "Plane" };
    int currentMeshType = (int)State->PreviewMeshType;
    if (ImGui::Combo("##PreviewMesh", &currentMeshType, meshTypes, 4))
    {
        State->PreviewMeshType = (EPreviewMeshType)currentMeshType;
        // TODO: Update preview mesh
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Material Parameters
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
    ImGui::Text("Parameters");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    if (State->MaterialInstance)
    {
        // TODO: Render scalar/vector/texture parameters
        ImGui::Text("Parameter editing coming soon...");
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Load a material to edit its parameters.");
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    ImGui::PopStyleVar();

    ImGui::End();

    if (!bViewerVisible)
    {
        CenterRect = FRect(0, 0, 0, 0);
        CenterRect.UpdateMinMax();
    }
}

void SMaterialEditorWindow::OnUpdate(float DeltaSeconds)
{
    if (!State || !State->Viewport)
        return;

    if (State->World)
    {
        State->World->Tick(DeltaSeconds);
    }

    if (State->Client)
    {
        State->Client->Tick(DeltaSeconds);
    }
}

void SMaterialEditorWindow::OnRenderViewport()
{
    if (State && State->Viewport && CenterRect.GetWidth() > 0 && CenterRect.GetHeight() > 0)
    {
        const uint32 NewStartX = static_cast<uint32>(CenterRect.Left);
        const uint32 NewStartY = static_cast<uint32>(CenterRect.Top);
        const uint32 NewWidth  = static_cast<uint32>(CenterRect.Right - CenterRect.Left);
        const uint32 NewHeight = static_cast<uint32>(CenterRect.Bottom - CenterRect.Top);
        State->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);

        State->Viewport->Render();
    }
}

void SMaterialEditorWindow::OnMouseMove(FVector2D MousePos)
{
    if (!State || !State->Viewport)
        return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
    }
}

void SMaterialEditorWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    if (!State || !State->Viewport)
        return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}

void SMaterialEditorWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (!State || !State->Viewport)
        return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}
