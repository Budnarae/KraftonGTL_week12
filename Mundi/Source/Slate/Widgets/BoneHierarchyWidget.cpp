#include "pch.h"
#include "BoneHierarchyWidget.h"
#include "ThirdParty/imgui/imgui.h"

FBoneHierarchyWidget::FBoneHierarchyWidget()
    : OnBoneSelected(nullptr)
    , TreeViewHeight(0.0f)
{
}

FBoneHierarchyWidget::~FBoneHierarchyWidget()
{
}

bool FBoneHierarchyWidget::Render(
    const FSkeleton* Skeleton,
    int32& InOutSelectedBoneIndex,
    std::set<int32>& InOutExpandedIndices
)
{
    bool bSelectionChanged = false;

    // 스켈레톤이 없거나 비어있는 경우
    if (!Skeleton)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("No skeletal mesh loaded.");
        ImGui::PopStyleColor();
        return false;
    }

    if (Skeleton->Bones.IsEmpty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextWrapped("This mesh has no skeleton data.");
        ImGui::PopStyleColor();
        return false;
    }

    // 스크롤 가능한 트리 뷰
    ImVec2 TreeSize(0, TreeViewHeight);
    ImGui::BeginChild("BoneTreeView", TreeSize, true);

    const TArray<FBone>& Bones = Skeleton->Bones;

    // 부모-자식 관계 배열 구축
    TArray<TArray<int32>> Children;
    Children.resize(Bones.size());
    for (int32 i = 0; i < Bones.size(); ++i)
    {
        int32 Parent = Bones[i].ParentIndex;
        if (Parent >= 0 && Parent < Bones.size())
        {
            Children[Parent].Add(i);
        }
    }

    // 루트 본들 렌더링 (ParentIndex가 -1인 본들)
    for (int32 i = 0; i < Bones.size(); ++i)
    {
        if (Bones[i].ParentIndex < 0)
        {
            RenderBoneNode(i, Bones, Children, InOutSelectedBoneIndex, InOutExpandedIndices, bSelectionChanged);
        }
    }

    ImGui::EndChild();

    // 선택 변경 시 콜백 호출
    if (bSelectionChanged && OnBoneSelected)
    {
        OnBoneSelected(InOutSelectedBoneIndex);
    }

    return bSelectionChanged;
}

void FBoneHierarchyWidget::RenderBoneNode(
    int32 BoneIndex,
    const TArray<FBone>& Bones,
    const TArray<TArray<int32>>& Children,
    int32& InOutSelectedBoneIndex,
    std::set<int32>& InOutExpandedIndices,
    bool& bOutSelectionChanged
)
{
    const bool bLeaf = Children[BoneIndex].IsEmpty();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;

    if (bLeaf)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    // 펼쳐진 노드는 명시적으로 열린 상태로 설정
    if (InOutExpandedIndices.count(BoneIndex) > 0)
    {
        ImGui::SetNextItemOpen(true);
    }

    // 선택된 노드 하이라이트
    if (InOutSelectedBoneIndex == BoneIndex)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID(BoneIndex);

    const char* Label = Bones[BoneIndex].Name.c_str();

    // 선택된 노드 색상 변경
    if (InOutSelectedBoneIndex == BoneIndex)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.55f, 0.85f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.40f, 0.60f, 0.90f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.30f, 0.50f, 0.80f, 1.0f));
    }

    bool open = ImGui::TreeNodeEx((void*)(intptr_t)BoneIndex, flags, "%s", Label ? Label : "<noname>");

    if (InOutSelectedBoneIndex == BoneIndex)
    {
        ImGui::PopStyleColor(3);

        // 선택된 본까지 스크롤
        ImGui::SetScrollHereY(0.5f);
    }

    // 사용자가 수동으로 노드를 접거나 펼쳤을 때 상태 업데이트
    if (ImGui::IsItemToggledOpen())
    {
        if (open)
            InOutExpandedIndices.insert(BoneIndex);
        else
            InOutExpandedIndices.erase(BoneIndex);
    }

    // 클릭 시 선택 변경
    if (ImGui::IsItemClicked())
    {
        if (InOutSelectedBoneIndex != BoneIndex)
        {
            InOutSelectedBoneIndex = BoneIndex;
            bOutSelectionChanged = true;
        }
    }

    // 자식 노드 재귀 렌더링
    if (!bLeaf && open)
    {
        for (int32 ChildIndex : Children[BoneIndex])
        {
            RenderBoneNode(ChildIndex, Bones, Children, InOutSelectedBoneIndex, InOutExpandedIndices, bOutSelectionChanged);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void FBoneHierarchyWidget::ExpandToNode(
    int32 BoneIndex,
    const FSkeleton* Skeleton,
    std::set<int32>& InOutExpandedIndices
)
{
    if (!Skeleton || BoneIndex < 0 || BoneIndex >= Skeleton->Bones.size())
        return;

    // 부모 경로를 따라 올라가며 모두 펼침
    int32 CurrentIndex = BoneIndex;
    while (CurrentIndex >= 0)
    {
        InOutExpandedIndices.insert(CurrentIndex);
        CurrentIndex = Skeleton->Bones[CurrentIndex].ParentIndex;
    }
}

void FBoneHierarchyWidget::SetOnBoneSelectedCallback(std::function<void(int32)> Callback)
{
    OnBoneSelected = Callback;
}

void FBoneHierarchyWidget::SetTreeViewHeight(float Height)
{
    TreeViewHeight = Height;
}
