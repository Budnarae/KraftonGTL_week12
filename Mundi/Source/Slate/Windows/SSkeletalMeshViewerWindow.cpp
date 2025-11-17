#include "pch.h"
#include "SSkeletalMeshViewerWindow.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "Source/Runtime/Engine/SkeletalViewer/SkeletalViewerBootstrap.h"
#include "Source/Editor/PlatformProcess.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Engine/Components/LineComponent.h"
#include "SelectionManager.h"
#include "USlateManager.h"
#include "BoneAnchorComponent.h"
#include "Source/Runtime/Engine/Collision/Picking.h"
#include "Source/Runtime/Engine/GameFramework/CameraActor.h"

SSkeletalMeshViewerWindow::SSkeletalMeshViewerWindow()
{
    CenterRect = FRect(0, 0, 0, 0);
}

SSkeletalMeshViewerWindow::~SSkeletalMeshViewerWindow()
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

// 베이스 클래스 Initialize 오버라이드 (기본 크기/위치 사용)
bool SSkeletalMeshViewerWindow::Initialize(ID3D11Device* InDevice, UWorld* InWorld)
{
    // 기본 위치와 크기
    const float DefaultWidth = 1200.0f;
    const float DefaultHeight = 800.0f;
    const float StartX = 200.0f;  // 화면 왼쪽에서 200px
    const float StartY = 100.0f;  // 화면 위에서 100px

    return Initialize(StartX, StartY, DefaultWidth, DefaultHeight, InWorld, InDevice);
}

// 커스텀 Initialize (위치/크기 지정)
bool SSkeletalMeshViewerWindow::Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice)
{
    // 베이스 클래스 초기화
    if (!SViewerWindowBase::Initialize(InDevice, InWorld))
        return false;

    SetRect(StartX, StartY, StartX + Width, StartY + Height);

    // Create first tab/state
    OpenNewTab("Viewer 1");
    ViewerState* State = static_cast<ViewerState*>(ActiveState);
    if (State && State->Viewport)
    {
        State->Viewport->Resize((uint32)StartX, (uint32)StartY, (uint32)Width, (uint32)Height);
    }

    bRequestFocus = true;
    return true;
}

void SSkeletalMeshViewerWindow::OnRender()
{
    // If window is closed, don't render
    if (!bIsOpen)
    {
        return;
    }

    // Parent detachable window (movable, top-level) with solid background
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;

    if (!bInitialPlacementDone)
    {
        ImGui::SetNextWindowPos(ImVec2(Rect.Left, Rect.Top));
        ImGui::SetNextWindowSize(ImVec2(Rect.GetWidth(), Rect.GetHeight()));
        bInitialPlacementDone = true;
    }

    if (bRequestFocus)
    {
        ImGui::SetNextWindowFocus();
    }
    bool bViewerVisible = false;
    if (ImGui::Begin("Skeletal Mesh Viewer", &bIsOpen, flags))
    {
        bViewerVisible = true;

        // 베이스 클래스의 탭 바 렌더링
        RenderTabBar();
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        Rect.Left = pos.x; Rect.Top = pos.y; Rect.Right = pos.x + size.x; Rect.Bottom = pos.y + size.y; Rect.UpdateMinMax();

        ImVec2 contentAvail = ImGui::GetContentRegionAvail();
        float totalWidth = contentAvail.x;
        float totalHeight = contentAvail.y;

        float leftWidth = totalWidth * LeftPanelRatio;
        float rightWidth = totalWidth * RightPanelRatio;
        float centerWidth = totalWidth - leftWidth - rightWidth;

        // Remove spacing between panels
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        // Left panel - Asset Browser & Bone Hierarchy
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();

        ViewerState* State = static_cast<ViewerState*>(ActiveState);
        if (State)
        {
            // Asset Browser Section
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
            ImGui::Indent(8.0f);
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::Text("Asset Browser");
            ImGui::PopFont();
            ImGui::Unindent(8.0f);
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();

            // === Asset Browser Widget (리팩토링됨) ===
            float buttonWidth = (leftWidth - 24.0f) * 0.5f - 4.0f;

            // 현재 경로를 위젯에 반영 (처음에만)
            if (AssetBrowser.GetPath()[0] == '\0' && State->MeshPathBuffer[0] != '\0')
            {
                AssetBrowser.SetPath(State->MeshPathBuffer);
            }

            // Asset Browser 렌더링 (Load 버튼 클릭 시 true 반환)
            if (AssetBrowser.Render("fbx", "FBX Files", buttonWidth))
            {
                // 로드 성공 시 처리
                FString Path = AssetBrowser.GetPath();
                USkeletalMesh* Mesh = UResourceManager::GetInstance().Load<USkeletalMesh>(Path);
                if (Mesh && State->PreviewActor)
                {
                    State->PreviewActor->SetSkeletalMesh(Path);
                    State->CurrentMesh = Mesh;
                    State->LoadedMeshPath = Path;  // Track for resource unloading

                    // 경로를 State에 반영
                    strncpy_s(State->MeshPathBuffer, Path.c_str(), sizeof(State->MeshPathBuffer) - 1);

                    if (auto* Skeletal = State->PreviewActor->GetSkeletalMeshComponent())
                    {
                        Skeletal->SetVisibility(State->bShowMesh);
                    }
                    State->bBoneLinesDirty = true;
                    if (auto* LineComp = State->PreviewActor->GetBoneLineComponent())
                    {
                        LineComp->ClearLines();
                        LineComp->SetLineVisible(State->bShowBones);
                    }
                }
            }

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();

            // Display Options
            ImGui::BeginGroup();
            ImGui::Text("Display Options:");
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.25f, 0.30f, 0.35f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.40f, 0.70f, 1.00f, 1.0f));

            if (ImGui::Checkbox("Show Mesh", &State->bShowMesh))
            {
                if (State->PreviewActor && State->PreviewActor->GetSkeletalMeshComponent())
                {
                    State->PreviewActor->GetSkeletalMeshComponent()->SetVisibility(State->bShowMesh);
                }
            }

            ImGui::SameLine();
            if (ImGui::Checkbox("Show Bones", &State->bShowBones))
            {
                if (State->PreviewActor && State->PreviewActor->GetBoneLineComponent())
                {
                    State->PreviewActor->GetBoneLineComponent()->SetLineVisible(State->bShowBones);
                }
                if (State->bShowBones)
                {
                    State->bBoneLinesDirty = true;
                }
            }
            ImGui::PopStyleColor(2);
            ImGui::EndGroup();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();

            // === Bone Hierarchy Widget (리팩토링됨) ===
            ImGui::Text("Bone Hierarchy:");
            ImGui::Spacing();

            const FSkeleton* Skeleton = State->CurrentMesh ? State->CurrentMesh->GetSkeleton() : nullptr;

            // Bone Hierarchy Widget 렌더링
            if (BoneHierarchy.Render(Skeleton, State->SelectedBoneIndex, State->ExpandedBoneIndices))
            {
                // 본 선택 변경 시 처리
                State->bBoneLinesDirty = true;
                ExpandToSelectedBone(State, State->SelectedBoneIndex);

                if (State->PreviewActor && State->World)
                {
                    State->PreviewActor->RepositionAnchorToBone(State->SelectedBoneIndex);
                    if (USceneComponent* Anchor = State->PreviewActor->GetBoneGizmoAnchor())
                    {
                        State->World->GetSelectionManager()->SelectActor(State->PreviewActor);
                        State->World->GetSelectionManager()->SelectComponent(Anchor);
                    }
                }
            }
        }
        else
        {
            ImGui::EndChild();
            ImGui::End();
            return;
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0); // No spacing between panels

        // Center panel (viewport area) — draw with border to see the viewport area
        ImGui::BeginChild("SkeletalMeshViewport", ImVec2(centerWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 childPos = ImGui::GetWindowPos();
        ImVec2 childSize = ImGui::GetWindowSize();
        ImVec2 rectMin = childPos;
        ImVec2 rectMax(childPos.x + childSize.x, childPos.y + childSize.y);
        CenterRect.Left = rectMin.x; CenterRect.Top = rectMin.y; CenterRect.Right = rectMax.x; CenterRect.Bottom = rectMax.y; CenterRect.UpdateMinMax();
        ImGui::EndChild();

        ImGui::SameLine(0, 0); // No spacing between panels

        // Right panel - Bone Properties
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::BeginChild("RightPanel", ImVec2(rightWidth, totalHeight), true);
        ImGui::PopStyleVar();

        // Panel header
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
        ImGui::Indent(8.0f);
        ImGui::Text("Bone Properties");
        ImGui::Unindent(8.0f);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.35f, 0.45f, 0.60f, 0.7f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // === Bone Property Editor Widget (리팩토링됨) ===
        if (State && State->SelectedBoneIndex >= 0 && State->CurrentMesh)
        {
            const FSkeleton* Skeleton = State->CurrentMesh->GetSkeleton();
            if (Skeleton && State->SelectedBoneIndex < Skeleton->Bones.size())
            {
                const FBone& SelectedBone = Skeleton->Bones[State->SelectedBoneIndex];

                // 본의 현재 트랜스폼 가져오기 (편집 중이 아닐 때만)
                if (!PropertyEditor.IsEditing())
                {
                    UpdateBoneTransformFromSkeleton(State);
                }

                // Property Editor 렌더링
                FBonePropertyEditResult EditResult = PropertyEditor.Render(
                    SelectedBone.Name.c_str(),
                    State->EditBoneLocation,
                    State->EditBoneRotation,
                    State->EditBoneScale
                );

                // 트랜스폼 변경 시 처리
                if (EditResult.AnyChanged())
                {
                    ApplyBoneTransform(State);
                    State->bBoneLinesDirty = true;
                }

                // bBoneRotationEditing 플래그 동기화 (기존 코드 호환성)
                State->bBoneRotationEditing = PropertyEditor.IsEditing();
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("Select a bone from the hierarchy to edit its transform properties.");
            ImGui::PopStyleColor();
        }

        ImGui::EndChild(); // RightPanel

        // Pop the ItemSpacing style
        ImGui::PopStyleVar();
    }
    ImGui::End();

    // If collapsed or not visible, clear the center rect so we don't render a floating viewport
    if (!bViewerVisible)
    {
        CenterRect = FRect(0, 0, 0, 0);
        CenterRect.UpdateMinMax();
    }
    
    bRequestFocus = false;
}

void SSkeletalMeshViewerWindow::OnUpdate(float DeltaSeconds)
{
    ViewerState* State = static_cast<ViewerState*>(ActiveState);
    if (!State || !State->Viewport)
        return;

    // Tick the preview world so editor actors (e.g., gizmo) update visibility/state
    if (State->World)
    {
        State->World->Tick(DeltaSeconds);
        if (State->World->GetGizmoActor())
            State->World->GetGizmoActor()->ProcessGizmoModeSwitch();
    }

    if (State && State->Client)
    {
        State->Client->Tick(DeltaSeconds);
    }
}

void SSkeletalMeshViewerWindow::OnMouseMove(FVector2D MousePos)
{
    ViewerState* State = static_cast<ViewerState*>(ActiveState);
    if (!State || !State->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
    }
}

void SSkeletalMeshViewerWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    ViewerState* State = static_cast<ViewerState*>(ActiveState);
    if (!State || !State->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);

        // First, always try gizmo picking (pass to viewport)
        State->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);

        // Left click: if no gizmo was picked, try bone picking
        if (Button == 0 && State->PreviewActor && State->CurrentMesh && State->Client && State->World)
        {
            // Check if gizmo was picked by checking selection
            UActorComponent* SelectedComp = State->World->GetSelectionManager()->GetSelectedComponent();

            // Only do bone picking if gizmo wasn't selected
            if (!SelectedComp || !Cast<UBoneAnchorComponent>(SelectedComp))
            {
                // Get camera from viewport client
                ACameraActor* Camera = State->Client->GetCamera();
                if (Camera)
                {
                    // Get camera vectors
                    FVector CameraPos = Camera->GetActorLocation();
                    FVector CameraRight = Camera->GetRight();
                    FVector CameraUp = Camera->GetUp();
                    FVector CameraForward = Camera->GetForward();

                    // Calculate viewport-relative mouse position
                    FVector2D ViewportMousePos(MousePos.X - CenterRect.Left, MousePos.Y - CenterRect.Top);
                    FVector2D ViewportSize(CenterRect.GetWidth(), CenterRect.GetHeight());

                    // Generate ray from mouse position
                    FRay Ray = MakeRayFromViewport(
                        Camera->GetViewMatrix(),
                        Camera->GetProjectionMatrix(CenterRect.GetWidth() / CenterRect.GetHeight(), State->Viewport),
                        CameraPos,
                        CameraRight,
                        CameraUp,
                        CameraForward,
                        ViewportMousePos,
                        ViewportSize
                    );

                    // Try to pick a bone
                    float HitDistance;
                    int32 PickedBoneIndex = State->PreviewActor->PickBone(Ray, HitDistance);

                    if (PickedBoneIndex >= 0)
                    {
                        // Bone was picked
                        State->SelectedBoneIndex = PickedBoneIndex;
                        State->bBoneLinesDirty = true;

                        ExpandToSelectedBone(State, PickedBoneIndex);

                        // Move gizmo to the selected bone
                        State->PreviewActor->RepositionAnchorToBone(PickedBoneIndex);
                        if (USceneComponent* Anchor = State->PreviewActor->GetBoneGizmoAnchor())
                        {
                            State->World->GetSelectionManager()->SelectActor(State->PreviewActor);
                            State->World->GetSelectionManager()->SelectComponent(Anchor);
                        }
                    }
                    else
                    {
                        // No bone was picked - clear selection
                        State->SelectedBoneIndex = -1;
                        State->bBoneLinesDirty = true;

                        // Hide gizmo and clear selection
                        if (UBoneAnchorComponent* Anchor = State->PreviewActor->GetBoneGizmoAnchor())
                        {
                            Anchor->SetVisibility(false);
                            Anchor->SetEditability(false);
                        }
                        State->World->GetSelectionManager()->ClearSelection();
                    }
                }
            }
        }
    }
}

void SSkeletalMeshViewerWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    ViewerState* State = static_cast<ViewerState*>(ActiveState);
    if (!State || !State->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}

void SSkeletalMeshViewerWindow::OnRenderViewport()
{
    ViewerState* State = static_cast<ViewerState*>(ActiveState);
    if (State && State->Viewport && CenterRect.GetWidth() > 0 && CenterRect.GetHeight() > 0)
    {
        const uint32 NewStartX = static_cast<uint32>(CenterRect.Left);
        const uint32 NewStartY = static_cast<uint32>(CenterRect.Top);
        const uint32 NewWidth  = static_cast<uint32>(CenterRect.Right - CenterRect.Left);
        const uint32 NewHeight = static_cast<uint32>(CenterRect.Bottom - CenterRect.Top);
        State->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);

        // 본 오버레이 재구축
        if (State->bShowBones)
        {
            State->bBoneLinesDirty = true;
        }
        if (State->bShowBones && State->PreviewActor && State->CurrentMesh && State->bBoneLinesDirty)
        {
            if (ULineComponent* LineComp = State->PreviewActor->GetBoneLineComponent())
            {
                LineComp->SetLineVisible(true);
            }
            State->PreviewActor->RebuildBoneLines(State->SelectedBoneIndex);
            State->bBoneLinesDirty = false;
        }

        // 뷰포트 렌더링 (ImGui보다 먼저)
        State->Viewport->Render();
    }
}

void SSkeletalMeshViewerWindow::LoadSkeletalMesh(const FString& Path)
{
    ViewerState* State = static_cast<ViewerState*>(ActiveState);
    if (!State || Path.empty())
        return;

    // Load the skeletal mesh using the resource manager
    USkeletalMesh* Mesh = UResourceManager::GetInstance().Load<USkeletalMesh>(Path);
    if (Mesh && State->PreviewActor)
    {
        // Set the mesh on the preview actor
        State->PreviewActor->SetSkeletalMesh(Path);
        State->CurrentMesh = Mesh;
        State->LoadedMeshPath = Path;  // Track for resource unloading

        // Update mesh path buffer for display in UI
        strncpy_s(State->MeshPathBuffer, Path.c_str(), sizeof(State->MeshPathBuffer) - 1);

        // Sync mesh visibility with checkbox state
        if (auto* Skeletal = State->PreviewActor->GetSkeletalMeshComponent())
        {
            Skeletal->SetVisibility(State->bShowMesh);
        }

        // Mark bone lines as dirty to rebuild on next frame
        State->bBoneLinesDirty = true;

        // Clear and sync bone line visibility
        if (auto* LineComp = State->PreviewActor->GetBoneLineComponent())
        {
            LineComp->ClearLines();
            LineComp->SetLineVisible(State->bShowBones);
        }

        UE_LOG("SSkeletalMeshViewerWindow: Loaded skeletal mesh from %s", Path.c_str());
    }
    else
    {
        UE_LOG("SSkeletalMeshViewerWindow: Failed to load skeletal mesh from %s", Path.c_str());
    }
}

void SSkeletalMeshViewerWindow::LoadAsset(const FString& AssetPath)
{
    LoadSkeletalMesh(AssetPath);
}

ViewerTabStateBase* SSkeletalMeshViewerWindow::CreateTabState(const char* Name)
{
    ViewerState* State = SkeletalViewerBootstrap::CreateViewerState(Name, World, Device);
    return State;
}

void SSkeletalMeshViewerWindow::DestroyTabState(ViewerTabStateBase* State)
{
    if (!State) return;
    ViewerState* SkeletalState = static_cast<ViewerState*>(State);
    SkeletalViewerBootstrap::DestroyViewerState(SkeletalState);
}

void SSkeletalMeshViewerWindow::UpdateBoneTransformFromSkeleton(ViewerState* State)
{
    if (!State || !State->CurrentMesh || State->SelectedBoneIndex < 0)
        return;

    // 본의 로컬 트랜스폼에서 값 추출
    const FTransform& BoneTransform = State->PreviewActor->GetSkeletalMeshComponent()->GetBoneLocalTransform(State->SelectedBoneIndex);
    State->EditBoneLocation = BoneTransform.Translation;
    State->EditBoneRotation = BoneTransform.Rotation.ToEulerZYXDeg();
    State->EditBoneScale = BoneTransform.Scale3D;
}

void SSkeletalMeshViewerWindow::ApplyBoneTransform(ViewerState* State)
{
    if (!State || !State->CurrentMesh || State->SelectedBoneIndex < 0)
        return;

    FTransform NewTransform(State->EditBoneLocation, FQuat::MakeFromEulerZYX(State->EditBoneRotation), State->EditBoneScale);
    State->PreviewActor->GetSkeletalMeshComponent()->SetBoneLocalTransform(State->SelectedBoneIndex, NewTransform);
}

void SSkeletalMeshViewerWindow::ExpandToSelectedBone(ViewerState* State, int32 BoneIndex)
{
    if (!State || !State->CurrentMesh)
        return;

    const FSkeleton* Skeleton = State->CurrentMesh->GetSkeleton();
    if (!Skeleton || BoneIndex < 0 || BoneIndex >= Skeleton->Bones.size())
        return;

    // 선택된 본부터 루트까지 모든 부모를 펼침
    int32 CurrentIndex = BoneIndex;
    while (CurrentIndex >= 0)
    {
        State->ExpandedBoneIndices.insert(CurrentIndex);
        CurrentIndex = Skeleton->Bones[CurrentIndex].ParentIndex;
    }
}
