#include "pch.h"
#include "SParticleEditWindow.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleViewerBootstrap.h"
#include "USlateManager.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleRequired.h"
#include "Source/Slate/Widgets/PropertyRenderer.h"
#include "Source/Runtime/Engine/Particle/ParticleAsset.h"
#include "Source/Runtime/Engine/Particle/ParticleSystem.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"

ImVec2 TopMenuBarOffset = ImVec2(0, 30);
ImVec2 TopMenuBarSize = ImVec2(-1, 40);


//enum, bool, int, float, vector2,3,4, TArray 정도 적용 필요
//어처피 프로퍼티는 생성되어 있음 그 프로퍼티정보를 가져와서 만들자
//근데 enum은 범위 정보가 없다 일단 제외


void SParticleEditWindow::CreateParticleEditor(const FString& Path)
{
    std::filesystem::path FolderPath = GContentDir + "/Resources/Particle";
    UParticleAsset::Create(FolderPath.string());

    auto* Viewer = USlateManager::GetInstance().FindViewer<SParticleEditWindow>();
    if (!Viewer || !Viewer->IsOpen())
    {
        // Open viewer with the currently selected skeletal mesh if available
        USlateManager::GetInstance().OpenViewer<SParticleEditWindow>(Path);
    }
}
void SParticleEditWindow::CreateParticleEditor(const UParticleModuleRequired* ParticleModule)
{
    auto* Viewer = USlateManager::GetInstance().FindViewer<SParticleEditWindow>();
    if (!Viewer || !Viewer->IsOpen())
    {
        // Open viewer with the currently selected skeletal mesh if available
        USlateManager::GetInstance().OpenViewer<SParticleEditWindow>();
    }
}
SParticleEditWindow::SParticleEditWindow()
{
    CenterRect = FRect(0, 0, 0, 0);
}

SParticleEditWindow::~SParticleEditWindow()
{

}
// 베이스 클래스 Initialize 오버라이드 (기본 크기/위치 사용)
bool SParticleEditWindow::Initialize(ID3D11Device* InDevice, UWorld* InWorld)
{
    // 기본 위치와 크기
    const float DefaultWidth = 1200.0f;
    const float DefaultHeight = 800.0f;
    const float StartX = 200.0f;  // 화면 왼쪽에서 200px
    const float StartY = 100.0f;  // 화면 위에서 100px

    return Initialize(StartX, StartY, DefaultWidth, DefaultHeight, InWorld, InDevice);
}

// 커스텀 Initialize (위치/크기 지정)
bool SParticleEditWindow::Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice)
{
    // 베이스 클래스 초기화
    if (!SViewerWindowBase::Initialize(InDevice, InWorld))
        return false;

    SetRect(StartX, StartY, StartX + Width, StartY + Height);

    // Create viewer state
    State = ParticleViewerBootstrap::CreateViewerState("ParticleEditor", InWorld, InDevice);
    if (State && State->Viewport)
    {
        State->Viewport->Resize((uint32)StartX, (uint32)StartY, (uint32)Width, (uint32)Height);
    }

    bRequestFocus = true;
    return true;
}    
void SParticleEditWindow::LoadAsset(const FString& AssetPath)
{

}
void SParticleEditWindow::OnRender()
{
    // If window is closed, don't render
    if (!bIsOpen)
    {
        return;
    }
    ImGuiWindowFlags ParentFlag = ImGuiWindowFlags_NoSavedSettings;
    ImGuiWindowFlags ChildFlag = ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse;

    if (!bInitialPlacementDone)
    {
        ImGui::SetNextWindowPos(ImVec2(Rect.Left, Rect.Top));
        ImGui::SetNextWindowSize(ImVec2(Rect.GetWidth(), Rect.GetHeight()));
        bInitialPlacementDone = true;
    }

    ImVec2 Size = ImVec2(Rect.GetWidth(), Rect.GetHeight() - 70);
    ImVec2 LTop = ImVec2(Rect.Left, Rect.Top + 70);

    if (ImGui::Begin("Particle Editor", &bIsOpen, ParentFlag))
    {
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        Rect.Left = pos.x; Rect.Top = pos.y; Rect.Right = pos.x + size.x; Rect.Bottom = pos.y + size.y; Rect.UpdateMinMax();
        if (ImGui::Button("Save"))
        {

        }
        ImGui::SameLine();
        if (ImGui::Button("Restart"))
        {

        }


        ImVec2 ChildSize = Size * 0.5f;
        ImGui::BeginChild("Viewport", ChildSize);
        //World Rendering
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("Emitter", ChildSize);
        //
        ImGui::EndChild();

        ImGui::BeginChild("Detail", ChildSize);
        /*UPropertyRenderer::RenderAllProperties();
        UClass* Class = UParticleModuleRequired::StaticClass();
        const TArray<FProperty>& Properties = Class->GetAllProperties();*/
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("CurveEditor", ChildSize);
        ImGui::EndChild();

      
        ImGui::End();
    }

}
void SParticleEditWindow::OnUpdate(float DeltaSeconds)
{
    if (!State || !State->Viewport)
        return;

    if (State && State->Client)
    {
        State->Client->Tick(DeltaSeconds);
    }
}

void SParticleEditWindow::OnMouseMove(FVector2D MousePos)
{
    if (!State || !State->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);
        State->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
    }
}

void SParticleEditWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    if (!State || !State->Viewport) return;

    if (CenterRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(CenterRect.Left, CenterRect.Top);

        // First, always try gizmo picking (pass to viewport)
        State->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);


    }
}

void SParticleEditWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (!State || !State->Viewport) return;


}

void SParticleEditWindow::OnRenderViewport()
{
    if (State && State->Viewport && CenterRect.GetWidth() > 0 && CenterRect.GetHeight() > 0)
    {
        const uint32 NewStartX = static_cast<uint32>(CenterRect.Left);
        const uint32 NewStartY = static_cast<uint32>(CenterRect.Top);
        const uint32 NewWidth = static_cast<uint32>(CenterRect.Right - CenterRect.Left);
        const uint32 NewHeight = static_cast<uint32>(CenterRect.Bottom - CenterRect.Top);
        State->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);

       

        // 뷰포트 렌더링 (ImGui보다 먼저)
        State->Viewport->Render();
    }
}
