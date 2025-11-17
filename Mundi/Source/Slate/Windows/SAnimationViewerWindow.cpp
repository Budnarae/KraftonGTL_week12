#include "pch.h"
#include "SAnimationViewerWindow.h"
#include "Source/Runtime/Engine/AnimationViewer/AnimationViewerBootstrap.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/AssetManagement/SkeletalMesh.h"
#include "Source/Runtime/Engine/Animation/AnimationSequence.h"
#include "Source/Runtime/Engine/Animation/AnimSingleNodeInstance.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "SelectionManager.h"
#include "Source/Runtime/Engine/Collision/Picking.h"
#include "Source/Runtime/Engine/GameFramework/CameraActor.h"
#include "BoneAnchorComponent.h"
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
    else if (AssetPath.find(".uanim") != FString::npos)
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

        // 본 라인 초기화
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

        // Reset bone line cache when loading new mesh
        State->PreviewActor->ResetBoneLinesCache();

        // Reset bone selection state when loading new mesh
        SelectedBoneIndex = -1;
        ExpandedBoneIndices.clear();

        // Hide and clear gizmo anchor
        if (UBoneAnchorComponent* Anchor = State->PreviewActor->GetBoneGizmoAnchor())
        {
            Anchor->SetVisibility(false);
            Anchor->SetEditability(false);
        }

        // Clear selection manager
        if (State->World)
        {
            State->World->GetSelectionManager()->ClearSelection();
        }

        // 드롭다운에서 선택된 인덱스 업데이트 (확장자 제거하고 비교)
        TArray<FString> MeshPaths = UResourceManager::GetInstance().GetAllFilePaths<USkeletalMesh>();
        FString PathWithoutExt = RemoveExtension(Path);
        for (int i = 0; i < MeshPaths.Num(); i++)
        {
            FString MeshPathWithoutExt = RemoveExtension(MeshPaths[i]);
            if (MeshPathWithoutExt == PathWithoutExt)
            {
                SelectedMeshIndex = i;
                break;
            }
        }

        UE_LOG("SAnimationViewerWindow: Loaded skeletal mesh from %s", Path.c_str());
    }
}

void SAnimationViewerWindow::LoadAnimation(const FString& Path)
{
    AnimationViewerState* State = static_cast<AnimationViewerState*>(ActiveState);
    if (!State || Path.empty())
        return;

    // ResourceManager에 애니메이션은 파일명만으로 등록됨 (예: "James_mixamo.com")
    // 전체 경로에서 파일명 추출
    size_t lastSlash = Path.find_last_of("/\\");
    FString fileName = (lastSlash != FString::npos) ? Path.substr(lastSlash + 1) : Path;

    // 확장자 제거
    FString animKey = RemoveExtension(fileName);

    // Load animation sequence
    UAnimationSequence* Anim = UResourceManager::GetInstance().Load<UAnimationSequence>(animKey);
    if (Anim)
    {
        State->CurrentAnimation = Anim;
        State->LoadedAnimPath = Path;
        strncpy_s(State->AnimPathBuffer, Path.c_str(), sizeof(State->AnimPathBuffer) - 1);

        // 애니메이션 재생 상태 초기화
        State->CurrentTime = 0.0f;
        State->bIsPlaying = false;

        // DEBUG: 애니메이션 정보 출력
        float playLength = Anim->GetPlayLength();
        float frameRate = Anim->GetFrameRate();
        UE_LOG("SAnimationViewerWindow: Loaded animation from %s", Path.c_str());
        UE_LOG("  PlayLength: %.3f, FrameRate: %.3f", playLength, frameRate);
        UE_LOG("  DataModel: %p", Anim->GetDataModel());

        // 자동으로 스켈레탈 메시 추론 및 로드
        if (!State->CurrentMesh)
        {
            // 파일명에서 Base Name 추출
            // 예: "C:/Content/James_mixamo.com.uanim" → "James"

            // 1. 경로에서 파일명만 추출
            size_t lastSlash = Path.find_last_of("/\\");
            FString fileName = (lastSlash != FString::npos) ? Path.substr(lastSlash + 1) : Path;

            // 2. 확장자 제거
            size_t lastDot = fileName.find_last_of('.');
            if (lastDot != FString::npos)
            {
                fileName = fileName.substr(0, lastDot);
            }

            // 3. "_" 이전의 문자열 추출 (Base Name)
            size_t underscorePos = fileName.find('_');
            FString baseName = (underscorePos != FString::npos) ? fileName.substr(0, underscorePos) : fileName;

            // 4. 스켈레탈 메시 경로 생성 (FBXLoader와 동일한 키 형식 사용)
            FString meshKey = "Content/Resources/" + baseName;

            // 5. 이미 로드된 메시가 있는지 확인
            USkeletalMesh* Mesh = UResourceManager::GetInstance().Get<USkeletalMesh>(meshKey);
            if (Mesh)
            {
                // 이미 로드되어 있으면 그냥 설정만
                State->PreviewActor->SetSkeletalMesh(Mesh->GetFilePath());
                State->CurrentMesh = Mesh;
                State->LoadedMeshPath = Mesh->GetFilePath();
                strncpy_s(State->MeshPathBuffer, Mesh->GetFilePath().c_str(), sizeof(State->MeshPathBuffer) - 1);

                // 본 라인 초기화
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

                // 드롭다운 인덱스 업데이트
                TArray<FString> MeshPaths = UResourceManager::GetInstance().GetAllFilePaths<USkeletalMesh>();
                FString PathWithoutExt = RemoveExtension(Mesh->GetFilePath());
                for (int i = 0; i < MeshPaths.Num(); i++)
                {
                    FString MeshPathWithoutExt = RemoveExtension(MeshPaths[i]);
                    if (MeshPathWithoutExt == PathWithoutExt)
                    {
                        SelectedMeshIndex = i;
                        break;
                    }
                }

                UE_LOG("SAnimationViewerWindow: Auto-linked to existing skeletal mesh: %s", Mesh->GetFilePath().c_str());
            }
            else
            {
                UE_LOG("SAnimationViewerWindow: Could not find skeletal mesh with key: %s", meshKey.c_str());
            }
        }

        // 애니메이션을 스켈레탈 메시 컴포넌트에 설정
        if (State->PreviewActor && State->CurrentMesh)
        {
            auto* SkeletalMeshComp = State->PreviewActor->GetSkeletalMeshComponent();
            if (SkeletalMeshComp)
            {
                // Skeleton을 애니메이션에 설정 (필수!)
                const FSkeleton* Skeleton = State->CurrentMesh->GetSkeleton();
                if (Skeleton)
                {
                    Anim->SetSkeleton(*Skeleton);
                    UE_LOG("SAnimationViewerWindow: Set skeleton to animation (%d bones)", Skeleton->Bones.Num());
                }
                else
                {
                    UE_LOG("SAnimationViewerWindow: WARNING - No skeleton found!");
                }

                // AnimSingleNodeInstance 생성 및 설정
                auto* AnimInstance = SkeletalMeshComp->GetAnimInstanceClass();
                UAnimSingleNodeInstance* SingleNodeInstance = dynamic_cast<UAnimSingleNodeInstance*>(AnimInstance);

                if (!SingleNodeInstance)
                {
                    // 기존 AnimInstance가 SingleNode가 아니면 새로 생성
                    SingleNodeInstance = NewObject<UAnimSingleNodeInstance>();
                    SkeletalMeshComp->SetAnimInstanceClass(SingleNodeInstance);
                    UE_LOG("SAnimationViewerWindow: Created new AnimSingleNodeInstance");
                }

                // 애니메이션 재생
                SingleNodeInstance->PlayAnimation(Anim, State->bLooping);
                UE_LOG("SAnimationViewerWindow: Started playing animation");
            }
        }
        else
        {
            if (!State->PreviewActor)
                UE_LOG("SAnimationViewerWindow: No PreviewActor!");
            if (!State->CurrentMesh)
                UE_LOG("SAnimationViewerWindow: No CurrentMesh!");
        }

        // 드롭다운에서 선택된 인덱스 업데이트
        TArray<UAnimationSequence*> AllAnims = UResourceManager::GetInstance().GetAll<UAnimationSequence>();
        for (int i = 0; i < AllAnims.Num(); i++)
        {
            if (AllAnims[i] == Anim)
            {
                SelectedAnimIndex = i;
                break;
            }
        }
    }
    else
    {
        UE_LOG("SAnimationViewerWindow: Failed to load animation from %s", Path.c_str());
    }
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

    // === Skeletal Mesh ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    ImGui::Indent(8.0f);
    ImGui::Text("Skeletal Mesh");
    ImGui::Unindent(8.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Indent(8.0f);

    // Show Mesh / Show Bones 체크박스
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.25f, 0.3f, 1.0f));

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

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 로드된 스켈레탈 메시 목록 가져오기
        TArray<USkeletalMesh*> AllMeshes = UResourceManager::GetInstance().GetAll<USkeletalMesh>();
        TArray<FString> MeshPaths = UResourceManager::GetInstance().GetAllFilePaths<USkeletalMesh>();

        if (AllMeshes.Num() > 0)
        {
            // 현재 선택된 메시의 이름 표시 (파일명만)
            FString currentMeshName = "Select Mesh...";
            if (SelectedMeshIndex >= 0 && SelectedMeshIndex < MeshPaths.Num())
            {
                currentMeshName = MeshPaths[SelectedMeshIndex];
                size_t lastSlash = currentMeshName.find_last_of("/\\");
                if (lastSlash != FString::npos)
                    currentMeshName = currentMeshName.substr(lastSlash + 1);
            }

            ImGui::PushItemWidth(-1);
            if (ImGui::BeginCombo("##MeshCombo", currentMeshName.c_str()))
            {
                for (int i = 0; i < MeshPaths.Num(); i++)
                {
                    // 경로에서 파일명만 추출
                    FString displayName = MeshPaths[i];
                    size_t lastSlash = displayName.find_last_of("/\\");
                    if (lastSlash != FString::npos)
                        displayName = displayName.substr(lastSlash + 1);

                    bool isSelected = (SelectedMeshIndex == i);
                    if (ImGui::Selectable(displayName.c_str(), isSelected))
                    {
                        SelectedMeshIndex = i;
                        LoadSkeletalMesh(MeshPaths[i]);
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
        }
        else
        {
            ImGui::TextDisabled("No skeletal meshes loaded");
        }

    ImGui::Unindent(8.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === Animation ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::Text("Animation");
    ImGui::PopStyleColor();

    ImGui::Spacing();

    ImGui::Indent(8.0f);

        // 로드된 애니메이션 목록 가져오기
        TArray<UAnimationSequence*> AllAnims = UResourceManager::GetInstance().GetAll<UAnimationSequence>();

        if (AllAnims.Num() > 0)
        {
            // 현재 선택된 애니메이션의 이름 표시 (FilePath가 비어있으면 인덱스 표시)
            FString currentAnimName = "Select Animation...";
            if (SelectedAnimIndex >= 0 && SelectedAnimIndex < AllAnims.Num())
            {
                FString filePath = AllAnims[SelectedAnimIndex]->GetFilePath();
                if (!filePath.empty())
                {
                    // 경로에서 파일명만 추출
                    size_t lastSlash = filePath.find_last_of("/\\");
                    currentAnimName = (lastSlash != FString::npos) ? filePath.substr(lastSlash + 1) : filePath;
                }
                else
                {
                    currentAnimName = "Animation " + std::to_string(SelectedAnimIndex);
                }
            }

            ImGui::PushItemWidth(-1);
            if (ImGui::BeginCombo("##AnimCombo", currentAnimName.c_str()))
            {
                for (int i = 0; i < AllAnims.Num(); i++)
                {
                    // 표시할 이름 생성
                    FString displayName;
                    FString filePath = AllAnims[i]->GetFilePath();
                    if (!filePath.empty())
                    {
                        size_t lastSlash = filePath.find_last_of("/\\");
                        displayName = (lastSlash != FString::npos) ? filePath.substr(lastSlash + 1) : filePath;
                    }
                    else
                    {
                        displayName = "Animation " + std::to_string(i);
                    }

                    bool isSelected = (SelectedAnimIndex == i);
                    if (ImGui::Selectable(displayName.c_str(), isSelected))
                    {
                        SelectedAnimIndex = i;
                        // 이미 로드된 객체를 직접 사용
                        State->CurrentAnimation = AllAnims[i];
                        State->CurrentTime = 0.0f;
                        State->bIsPlaying = false;

                        // 스켈레탈 메시가 이미 로드되어 있으면 애니메이션 설정
                        if (State->PreviewActor && State->CurrentMesh)
                        {
                            auto* SkeletalMeshComp = State->PreviewActor->GetSkeletalMeshComponent();
                            if (SkeletalMeshComp)
                            {
                                // Skeleton 설정
                                const FSkeleton* Skeleton = State->CurrentMesh->GetSkeleton();
                                if (Skeleton)
                                {
                                    AllAnims[i]->SetSkeleton(*Skeleton);
                                }

                                // AnimSingleNodeInstance로 재생
                                auto* AnimInstance = SkeletalMeshComp->GetAnimInstanceClass();
                                UAnimSingleNodeInstance* SingleNodeInstance = dynamic_cast<UAnimSingleNodeInstance*>(AnimInstance);

                                if (!SingleNodeInstance)
                                {
                                    SingleNodeInstance = NewObject<UAnimSingleNodeInstance>();
                                    SkeletalMeshComp->SetAnimInstanceClass(SingleNodeInstance);
                                }

                                SingleNodeInstance->PlayAnimation(AllAnims[i], State->bLooping);
                                UE_LOG("SAnimationViewerWindow: Animation selected from dropdown");
                            }
                        }
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
        }
        else
    {
        ImGui::TextDisabled("No animations loaded");
    }

    ImGui::Unindent(8.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === Bone Hierarchy ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::Text("Bone Hierarchy");
    ImGui::PopStyleColor();

    ImGui::Spacing();

    ImGui::Indent(8.0f);
        const FSkeleton* Skeleton = State->CurrentMesh ? State->CurrentMesh->GetSkeleton() : nullptr;

        if (BoneHierarchy.Render(Skeleton, SelectedBoneIndex, ExpandedBoneIndices))
        {
            // 본 선택 변경 시 처리
            State->bBoneLinesDirty = true;
            ExpandToSelectedBone(State, SelectedBoneIndex);

            if (State->PreviewActor && State->World)
            {
                State->PreviewActor->RepositionAnchorToBone(SelectedBoneIndex);
                if (USceneComponent* Anchor = State->PreviewActor->GetBoneGizmoAnchor())
                {
                    State->World->GetSelectionManager()->SelectActor(State->PreviewActor);
                    State->World->GetSelectionManager()->SelectComponent(Anchor);
                }
        }
    }

    ImGui::Unindent(8.0f);

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

    // === Right Panel: Playback + Bone Transform Controls ===
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild("RightPanel", ImVec2(rightWidth, totalHeight), true);
    ImGui::PopStyleVar();

    // === Playback Controls ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    ImGui::Indent(8.0f);
    ImGui::Text("Playback");
    ImGui::Unindent(8.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Indent(8.0f);

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

    ImGui::Unindent(8.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === Bone Transform Editor ===
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.35f, 0.50f, 0.8f));
    ImGui::Text("Bone Transform");
    ImGui::PopStyleColor();

    ImGui::Spacing();

    ImGui::Indent(8.0f);

    if (State && SelectedBoneIndex >= 0 && State->CurrentMesh)
    {
        const FSkeleton* Skeleton = State->CurrentMesh->GetSkeleton();
        if (Skeleton && SelectedBoneIndex < Skeleton->Bones.size())
        {
            const FBone& SelectedBone = Skeleton->Bones[SelectedBoneIndex];

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

            // bBoneRotationEditing 플래그 동기화
            State->bBoneRotationEditing = PropertyEditor.IsEditing();
        }
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("Select a bone from the hierarchy to edit its transform.");
        ImGui::PopStyleColor();
    }

    ImGui::Unindent(8.0f);

    ImGui::EndChild();

    ImGui::PopStyleVar();

    // === Bottom Panel: Timeline ===
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild("Timeline", ImVec2(totalWidth, BottomPanelHeight), true);
    ImGui::PopStyleVar();

    if (State->CurrentAnimation)
    {
        float animLength = State->CurrentAnimation->GetPlayLength();

        // 디버그: 값 출력
        char debugText[128];
        snprintf(debugText, sizeof(debugText), "Time: %.2f / %.2f sec", State->CurrentTime, animLength);
        ImGui::Text("%s", debugText);

        ImGui::PushItemWidth(-1);
        ImGui::SliderFloat("##Timeline", &State->CurrentTime, 0.0f, animLength, "%.2f", ImGuiSliderFlags_AlwaysClamp);
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
    if (State->CurrentAnimation)
    {
        if (State->bIsPlaying)
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
        }

        // 애니메이션 시간 동기화 (타임라인 슬라이더를 통한 수동 변경 지원)
        UAnimationSequence* AnimSeq = dynamic_cast<UAnimationSequence*>(State->CurrentAnimation);
        if (AnimSeq)
        {
            AnimSeq->SetCurrentTime(State->CurrentTime);
        }
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
            State->PreviewActor->RebuildBoneLines(SelectedBoneIndex);
            State->bBoneLinesDirty = false;
        }

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
                        SelectedBoneIndex = PickedBoneIndex;
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
                        SelectedBoneIndex = -1;
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

void SAnimationViewerWindow::UpdateBoneTransformFromSkeleton(AnimationViewerState* State)
{
    if (!State || !State->CurrentMesh || SelectedBoneIndex < 0)
        return;

    // 본의 로컬 트랜스폼에서 값 추출
    if (State->PreviewActor && State->PreviewActor->GetSkeletalMeshComponent())
    {
        const FTransform& BoneTransform = State->PreviewActor->GetSkeletalMeshComponent()->GetBoneLocalTransform(SelectedBoneIndex);
        State->EditBoneLocation = BoneTransform.Translation;
        State->EditBoneRotation = BoneTransform.Rotation.ToEulerZYXDeg();
        State->EditBoneScale = BoneTransform.Scale3D;
    }
}

void SAnimationViewerWindow::ApplyBoneTransform(AnimationViewerState* State)
{
    if (!State || !State->CurrentMesh || SelectedBoneIndex < 0)
        return;

    if (State->PreviewActor && State->PreviewActor->GetSkeletalMeshComponent())
    {
        FTransform NewTransform(State->EditBoneLocation, FQuat::MakeFromEulerZYX(State->EditBoneRotation), State->EditBoneScale);
        State->PreviewActor->GetSkeletalMeshComponent()->SetBoneLocalTransform(SelectedBoneIndex, NewTransform);
    }
}

void SAnimationViewerWindow::ExpandToSelectedBone(AnimationViewerState* State, int32 BoneIndex)
{
    if (!State || !State->CurrentMesh)
        return;

    const FSkeleton* Skeleton = State->CurrentMesh->GetSkeleton();
    if (!Skeleton || BoneIndex < 0 || BoneIndex >= Skeleton->Bones.size())
        return;

    // 선택된 본의 모든 부모 본을 펼침
    int32 CurrentBone = BoneIndex;
    while (CurrentBone >= 0)
    {
        ExpandedBoneIndices.insert(CurrentBone);
        CurrentBone = Skeleton->Bones[CurrentBone].ParentIndex;
    }
}
