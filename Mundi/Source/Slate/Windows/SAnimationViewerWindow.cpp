#include "pch.h"
#include "SAnimationViewerWindow.h"
#include "Source/Runtime/Engine/AnimationViewer/AnimationViewerBootstrap.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/AssetManagement/SkeletalMesh.h"
#include "Source/Runtime/Engine/Animation/AnimationSequence.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "ThirdParty/imgui/imgui.h"

SAnimationViewerWindow::SAnimationViewerWindow()
    : SViewerWindowBase()
{
    CenterRect = FRect(0, 0, 0, 0);
}

SAnimationViewerWindow::~SAnimationViewerWindow()
{
    // 모든 탭 정리
    for (ViewerTabStateBase* Tab : Tabs)
    {
        if (Tab)
        {
            DestroyTabState(Tab);
        }
    }
    Tabs.Empty();
}

bool SAnimationViewerWindow::Initialize(ID3D11Device* InDevice, UWorld* InWorld)
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

    // 첫 번째 탭 생성
    OpenNewTab("Animation Viewer 1");

    // 뷰포트 크기 초기화
    AnimationViewerState* State = static_cast<AnimationViewerState*>(ActiveState);
    if (State && State->Viewport)
    {
        State->Viewport->Resize((uint32)StartX, (uint32)StartY, (uint32)DefaultWidth, (uint32)DefaultHeight);
    }

    bRequestFocus = true;
    return true;
}

ViewerTabStateBase* SAnimationViewerWindow::CreateTabState(const char* Name)
{
    return AnimationViewerBootstrap::CreateViewerState(Name, World, Device);
}

void SAnimationViewerWindow::DestroyTabState(ViewerTabStateBase* State)
{
    AnimationViewerState* AVState = static_cast<AnimationViewerState*>(State);
    AnimationViewerBootstrap::DestroyViewerState(AVState);
}

void SAnimationViewerWindow::LoadAsset(const FString& AssetPath)
{
    // Determine if it's a mesh or animation by extension
    if (AssetPath.find(".fbx") != FString::npos)
    {
        LoadSkeletalMesh(AssetPath);
    }
    else if (AssetPath.find(".anim") != FString::npos)
    {
        LoadAnimation(AssetPath);
    }
}

void SAnimationViewerWindow::LoadSkeletalMesh(const FString& Path)
{
    AnimationViewerState* State = static_cast<AnimationViewerState*>(ActiveState);
    if (!State || !State->PreviewActor)
        return;

    USkeletalMesh* Mesh = UResourceManager::GetInstance().Load<USkeletalMesh>(Path);
    if (Mesh)
    {
        State->PreviewActor->SetSkeletalMesh(Path);
        State->CurrentMesh = Mesh;
        State->LoadedMeshPath = Path;
        strncpy_s(State->MeshPathBuffer, Path.c_str(), sizeof(State->MeshPathBuffer) - 1);
    }
}

void SAnimationViewerWindow::LoadAnimation(const FString& Path)
{
    AnimationViewerState* State = static_cast<AnimationViewerState*>(ActiveState);
    if (!State)
        return;

    // TODO: Load animation sequence
    // UAnimationSequence* Anim = UResourceManager::GetInstance().Load<UAnimationSequence>(Path);
    // State->CurrentAnimation = Anim;
    // State->LoadedAnimPath = Path;
}

void SAnimationViewerWindow::OnRender()
{
    AnimationViewerState* State = static_cast<AnimationViewerState*>(ActiveState);
    if (!State)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    bool bViewerVisible = ImGui::Begin("Animation Viewer", &bIsOpen, window_flags);

    if (!bViewerVisible)
    {
        ImGui::End();
        CenterRect = FRect(0, 0, 0, 0);
        CenterRect.UpdateMinMax();
        return;
    }

    // 탭 바 렌더링
    RenderTabBar();

    // 메인 패널 레이아웃
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImVec2 availableRegion = ImGui::GetContentRegionAvail();
    float totalWidth = availableRegion.x;
    float totalHeight = availableRegion.y - BottomPanelHeight;  // 타임라인 공간 확보

    float leftWidth = totalWidth * LeftPanelRatio;
    float rightWidth = totalWidth * RightPanelRatio;
    float centerWidth = totalWidth - leftWidth - rightWidth;

    // === Left Panel: Asset Browser + Bone Hierarchy ===
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, totalHeight), true);
    ImGui::PopStyleVar();

    // Mesh Browser
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    ImGui::Indent(8.0f);
    ImGui::Text("Skeletal Mesh");
    ImGui::Unindent(8.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float buttonWidth = (leftWidth - 24.0f) * 0.5f - 4.0f;
    MeshBrowser.SetLoadButtonLabel("Load Mesh");
    MeshBrowser.SetPlaceholderText("Browse for FBX file...");

    if (MeshBrowser.Render("fbx", "FBX Files", buttonWidth))
    {
        LoadSkeletalMesh(MeshBrowser.GetPath());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Animation Browser
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::Text("Animation");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    AnimBrowser.SetLoadButtonLabel("Load Anim");
    AnimBrowser.SetPlaceholderText("Browse for animation file...");

    if (AnimBrowser.Render("anim", "Animation Files", buttonWidth))
    {
        LoadAnimation(AnimBrowser.GetPath());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Bone Hierarchy (재사용!)
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::Text("Bone Hierarchy");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    const FSkeleton* Skeleton = State->CurrentMesh ? State->CurrentMesh->GetSkeleton() : nullptr;
    BoneHierarchy.Render(Skeleton, SelectedBoneIndex, ExpandedBoneIndices);

    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    // === Center Panel: Viewport ===
    ImGui::BeginChild("AnimViewport", ImVec2(centerWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImVec2 childPos = ImGui::GetWindowPos();
    ImVec2 childSize = ImGui::GetWindowSize();
    CenterRect.Left = childPos.x;
    CenterRect.Top = childPos.y;
    CenterRect.Right = childPos.x + childSize.x;
    CenterRect.Bottom = childPos.y + childSize.y;
    CenterRect.UpdateMinMax();
    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    // === Right Panel: Playback Controls ===
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild("RightPanel", ImVec2(rightWidth, totalHeight), true);
    ImGui::PopStyleVar();

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    ImGui::Indent(8.0f);
    ImGui::Text("Playback");
    ImGui::Unindent(8.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (State->CurrentAnimation)
    {
        // Play/Pause/Stop buttons
        if (ImGui::Button(State->bIsPlaying ? "Pause" : "Play", ImVec2(100, 30)))
        {
            State->bIsPlaying = !State->bIsPlaying;
        }

        ImGui::SameLine();

        if (ImGui::Button("Stop", ImVec2(100, 30)))
        {
            State->bIsPlaying = false;
            State->CurrentTime = 0.0f;
        }

        ImGui::Spacing();

        // Playback speed
        ImGui::Text("Speed:");
        ImGui::SliderFloat("##Speed", &State->PlaybackSpeed, 0.1f, 2.0f);

        ImGui::Spacing();

        // Loop toggle
        ImGui::Checkbox("Loop", &State->bLooping);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Load an animation to preview it.");
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    ImGui::PopStyleVar();

    // === Bottom Panel: Timeline ===
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild("Timeline", ImVec2(totalWidth, BottomPanelHeight), true);
    ImGui::PopStyleVar();

    if (State->CurrentAnimation)
    {
        float animLength = State->CurrentAnimation->GetPlayLength();

        ImGui::Text("Timeline:");
        ImGui::PushItemWidth(-1);
        ImGui::SliderFloat("##Timeline", &State->CurrentTime, 0.0f, animLength, "%.2f / %.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::PopItemWidth();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Text("No animation loaded");
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    ImGui::End();

    if (!bViewerVisible)
    {
        CenterRect = FRect(0, 0, 0, 0);
        CenterRect.UpdateMinMax();
    }
}

void SAnimationViewerWindow::OnUpdate(float DeltaSeconds)
{
    AnimationViewerState* State = static_cast<AnimationViewerState*>(ActiveState);
    if (!State || !State->Viewport)
        return;

    // Update animation time
    if (State->bIsPlaying && State->CurrentAnimation)
    {
        State->CurrentTime += DeltaSeconds * State->PlaybackSpeed;

        float animLength = State->CurrentAnimation->GetPlayLength();
        if (State->CurrentTime >= animLength)
        {
            if (State->bLooping)
            {
                State->CurrentTime = fmod(State->CurrentTime, animLength);
            }
            else
            {
                State->CurrentTime = animLength;
                State->bIsPlaying = false;
            }
        }

        // TODO: Update skeletal mesh component with current animation pose
    }

    if (State->World)
    {
        State->World->Tick(DeltaSeconds);
    }

    if (State->Client)
    {
        State->Client->Tick(DeltaSeconds);
    }
}

void SAnimationViewerWindow::OnRenderViewport()
{
    AnimationViewerState* State = static_cast<AnimationViewerState*>(ActiveState);
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

void SAnimationViewerWindow::OnMouseMove(FVector2D MousePos)
{
    AnimationViewerState* State = static_cast<AnimationViewerState*>(ActiveState);
    if (!State || !State->Viewport)
        return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
    }
}

void SAnimationViewerWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    AnimationViewerState* State = static_cast<AnimationViewerState*>(ActiveState);
    if (!State || !State->Viewport)
        return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}

void SAnimationViewerWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    AnimationViewerState* State = static_cast<AnimationViewerState*>(ActiveState);
    if (!State || !State->Viewport)
        return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}
