#include "pch.h"
#include "SStaticMeshViewerWindow.h"

#include "FViewport.h"
#include "FViewportClient.h"
#include "Source/Runtime/Engine/StaticMeshViewer/StaticMeshViewerBootstrap.h"
#include "Source/Runtime/Engine/GameFramework/StaticMeshActor.h"
#include "Source/Runtime/Engine/Components/StaticMeshComponent.h"
#include "Source/Runtime/AssetManagement/StaticMesh.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include "ThirdParty/imgui/imgui.h"

SStaticMeshViewerWindow::SStaticMeshViewerWindow()
    : SViewerWindowBase()
{
    CenterRect = FRect(0, 0, 0, 0);
}

SStaticMeshViewerWindow::~SStaticMeshViewerWindow()
{
    if (State)
    {
        StaticMeshViewerBootstrap::DestroyViewerState(State);
        State = nullptr;
    }
}

bool SStaticMeshViewerWindow::Initialize(ID3D11Device* InDevice, UWorld* InWorld)
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

    // Create viewer state
    State = StaticMeshViewerBootstrap::CreateViewerState("Viewer", World, Device);
    if (State && State->Viewport)
    {
        State->Viewport->Resize((uint32)StartX, (uint32)StartY, (uint32)DefaultWidth, (uint32)DefaultHeight);
    }

    bRequestFocus = true;
    return true;
}

void SStaticMeshViewerWindow::LoadAsset(const FString& AssetPath)
{
    LoadStaticMesh(AssetPath);
}

void SStaticMeshViewerWindow::LoadStaticMesh(const FString& Path)
{
    if (!State || Path.empty())
        return;

    // Load the static mesh using the resource manager
    UStaticMesh* Mesh = UResourceManager::GetInstance().Load<UStaticMesh>(Path);
    if (Mesh && State->PreviewActor)
    {
        // Set the mesh on the preview actor (matches SkeletalMeshViewer pattern)
        State->PreviewActor->SetStaticMesh(Path);
        State->CurrentMesh = Mesh;
        State->LoadedMeshPath = Path;  // Track for resource unloading

        // Update mesh path buffer for display in UI
        strncpy_s(State->MeshPathBuffer, Path.c_str(), sizeof(State->MeshPathBuffer) - 1);

        UE_LOG("SStaticMeshViewerWindow: Loaded static mesh from %s", Path.c_str());
    }
    else
    {
        UE_LOG("SStaticMeshViewerWindow: Failed to load static mesh from %s", Path.c_str());
    }
}

void SStaticMeshViewerWindow::OnRender()
{
    if (!State)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    bool bViewerVisible = ImGui::Begin("Static Mesh Viewer", &bIsOpen, window_flags);

    if (!bViewerVisible)
    {
        ImGui::End();
        CenterRect = FRect(0, 0, 0, 0);
        CenterRect.UpdateMinMax();
        return;
    }

    // === 메인 패널 레이아웃 ===
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImVec2 availableRegion = ImGui::GetContentRegionAvail();
    float totalWidth = availableRegion.x;
    float totalHeight = availableRegion.y;

    float leftWidth = totalWidth * LeftPanelRatio;
    float rightWidth = totalWidth * RightPanelRatio;
    float centerWidth = totalWidth - leftWidth - rightWidth;

    // === Left Panel: Asset Browser ===
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, totalHeight), true);
    ImGui::PopStyleVar();

    // Panel header
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    ImGui::Indent(8.0f);
    ImGui::Text("Asset Browser");
    ImGui::Unindent(8.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Asset Browser Widget
    float buttonWidth = (leftWidth - 24.0f) * 0.5f - 4.0f;

    if (AssetBrowser.GetPath()[0] == '\0' && State->MeshPathBuffer[0] != '\0')
    {
        AssetBrowser.SetPath(State->MeshPathBuffer);
    }

    AssetBrowser.SetLoadButtonLabel("Load Mesh");
    AssetBrowser.SetPlaceholderText("Browse for mesh file...");

    if (AssetBrowser.Render("umesh", "Static Mesh Files", buttonWidth))
    {
        LoadStaticMesh(AssetBrowser.GetPath());
    }

    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    // === Center Panel: Viewport ===
    ImGui::BeginChild("StaticMeshViewport", ImVec2(centerWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImVec2 childPos = ImGui::GetWindowPos();
    ImVec2 childSize = ImGui::GetWindowSize();
    ImVec2 rectMin = childPos;
    ImVec2 rectMax(childPos.x + childSize.x, childPos.y + childSize.y);
    CenterRect.Left = rectMin.x;
    CenterRect.Top = rectMin.y;
    CenterRect.Right = rectMax.x;
    CenterRect.Bottom = rectMax.y;
    CenterRect.UpdateMinMax();
    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    // === Right Panel: Properties ===
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild("RightPanel", ImVec2(rightWidth, totalHeight), true);
    ImGui::PopStyleVar();

    // Panel header
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    ImGui::Indent(8.0f);
    ImGui::Text("Mesh Properties");
    ImGui::Unindent(8.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    if (State->CurrentMesh)
    {
        // Mesh Info
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.90f, 0.40f, 1.0f));
        ImGui::Text("Mesh Information");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::Text("Vertices: %u", State->CurrentMesh->GetVertexCount());
        ImGui::Text("Triangles: %u", State->CurrentMesh->GetIndexCount() / 3);

        // GetMeshGroupCount()는 StaticMeshAsset에 접근하므로 nullptr 체크 필요
        if (State->CurrentMesh->GetStaticMeshAsset())
        {
            ImGui::Text("Groups: %llu", State->CurrentMesh->GetMeshGroupCount());
        }
        else
        {
            ImGui::Text("Groups: 0");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Display Options
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("Display Options");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::Checkbox("Wireframe", &State->bShowWireframe);
        ImGui::Checkbox("Bounds", &State->bShowBounds);
        ImGui::Checkbox("Collision", &State->bShowCollision);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Load a static mesh to view its properties.");
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

void SStaticMeshViewerWindow::OnUpdate(float DeltaSeconds)
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

void SStaticMeshViewerWindow::OnRenderViewport()
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

void SStaticMeshViewerWindow::OnMouseMove(FVector2D MousePos)
{
    if (!State || !State->Viewport)
        return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
    }
}

void SStaticMeshViewerWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    if (!State || !State->Viewport)
        return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}

void SStaticMeshViewerWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (!State || !State->Viewport)
        return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}
