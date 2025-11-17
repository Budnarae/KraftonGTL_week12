#include "pch.h"
#include "BonePropertyEditor.h"
#include "ThirdParty/imgui/imgui.h"

FBonePropertyEditor::FBonePropertyEditor()
    : OnTransformChanged(nullptr)
    , bIsEditing(false)
    , LocationDragSpeed(0.1f)
    , RotationDragSpeed(0.5f)
    , ScaleDragSpeed(0.01f)
{
}

FBonePropertyEditor::~FBonePropertyEditor()
{
}

FBonePropertyEditResult FBonePropertyEditor::Render(
    const char* BoneName,
    FVector& InOutLocation,
    FVector& InOutRotation,
    FVector& InOutScale
)
{
    FBonePropertyEditResult Result;

    // 본 이름 헤더
    if (BoneName)
    {
        RenderBoneNameOnly(BoneName);
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.45f, 0.55f, 0.70f, 0.8f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    // === Location 편집 ===
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
    ImGui::Text("Location");
    ImGui::PopStyleColor();

    ImGui::PushItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.28f, 0.20f, 0.20f, 0.6f));

    Result.bLocationChanged |= ImGui::DragFloat("##BoneLocX", &InOutLocation.X, LocationDragSpeed, 0.0f, 0.0f, "X: %.3f");
    Result.bLocationChanged |= ImGui::DragFloat("##BoneLocY", &InOutLocation.Y, LocationDragSpeed, 0.0f, 0.0f, "Y: %.3f");
    Result.bLocationChanged |= ImGui::DragFloat("##BoneLocZ", &InOutLocation.Z, LocationDragSpeed, 0.0f, 0.0f, "Z: %.3f");

    ImGui::PopStyleColor();
    ImGui::PopItemWidth();
    ImGui::Spacing();

    // === Rotation 편집 ===
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.5f, 1.0f));
    ImGui::Text("Rotation");
    ImGui::PopStyleColor();

    // 편집 중 상태 감지
    if (ImGui::IsAnyItemActive())
    {
        bIsEditing = true;
    }

    ImGui::PushItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.28f, 0.20f, 0.6f));

    Result.bRotationChanged |= ImGui::DragFloat("##BoneRotX", &InOutRotation.X, RotationDragSpeed, -180.0f, 180.0f, "X: %.2f°");
    Result.bRotationChanged |= ImGui::DragFloat("##BoneRotY", &InOutRotation.Y, RotationDragSpeed, -180.0f, 180.0f, "Y: %.2f°");
    Result.bRotationChanged |= ImGui::DragFloat("##BoneRotZ", &InOutRotation.Z, RotationDragSpeed, -180.0f, 180.0f, "Z: %.2f°");

    ImGui::PopStyleColor();
    ImGui::PopItemWidth();

    if (!ImGui::IsAnyItemActive())
    {
        bIsEditing = false;
    }

    ImGui::Spacing();

    // === Scale 편집 ===
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 1.0f, 1.0f));
    ImGui::Text("Scale");
    ImGui::PopStyleColor();

    ImGui::PushItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.20f, 0.20f, 0.28f, 0.6f));

    Result.bScaleChanged |= ImGui::DragFloat("##BoneScaleX", &InOutScale.X, ScaleDragSpeed, 0.001f, 100.0f, "X: %.3f");
    Result.bScaleChanged |= ImGui::DragFloat("##BoneScaleY", &InOutScale.Y, ScaleDragSpeed, 0.001f, 100.0f, "Y: %.3f");
    Result.bScaleChanged |= ImGui::DragFloat("##BoneScaleZ", &InOutScale.Z, ScaleDragSpeed, 0.001f, 100.0f, "Z: %.3f");

    ImGui::PopStyleColor();
    ImGui::PopItemWidth();

    // 변경 시 콜백 호출
    if (Result.AnyChanged() && OnTransformChanged)
    {
        OnTransformChanged();
    }

    return Result;
}

void FBonePropertyEditor::RenderBoneNameOnly(const char* BoneName)
{
    // Selected bone header
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.90f, 0.40f, 1.0f));
    ImGui::Text("> Selected Bone");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.95f, 1.00f, 1.0f));
    ImGui::TextWrapped("%s", BoneName);
    ImGui::PopStyleColor();
}

void FBonePropertyEditor::SetOnTransformChangedCallback(std::function<void()> Callback)
{
    OnTransformChanged = Callback;
}
