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
#include "Source/Runtime/Engine/Particle/ParticleModuleColor.h"

ImVec2 TopMenuBarOffset = ImVec2(0, 30);
ImVec2 TopMenuBarSize = ImVec2(-1, 40);

//Dropdown에서 첫 카테고리안에 두번째 생성할 목록이 들어가 있다.
//각 생성할 목록들을 클릭하면 그 값이 현재 클릭된 이미터를 기준으로 추가된다.
//즉 파티클 시스템에 추가 함수가 있고, 각 목록들은 그함수에 연결되어 있다.
//함수 연결? 아니면 string?
//함수연결이 낫겠지


void FMenuAction::Action(SParticleEditWindow* ParticleEditWindow) const
{
    switch (MenuActionType)
    {
    case EMenuActionType::AddEmitter:
        ParticleEditWindow->AddEmitter(EmitterOffset);
        break;
    case EMenuActionType::AddSpawnModule:
        ParticleEditWindow->AddSpawnModule(ClassName);
        break;
    case EMenuActionType::AddUpdateModule:
        ParticleEditWindow->AddUpdateModule(ClassName);
        break;
    }
}

TMap<FString, TMap<FString, FMenuAction>> DropdownActionMap =
{
    {"파티클 시스템",
    {
        {"앞에 이미터 추가", FMenuAction::CreateAddEmitter(-1)},
        {"뒤에 이미터 추가", FMenuAction::CreateAddEmitter(1)},
    }
    },
    {"컬러",
    {
        {"초기컬러", FMenuAction::CreateSpawnModule(UParticleModuleColor::StaticClass()->Name)},
        {"컬러 오버 라이프", FMenuAction::CreateUpdateModule(UParticleModuleColor::StaticClass()->Name)},
    }
    }
};


void SParticleEditWindow::AddEmitter(const int EmitterOffset)
{
    if (State->PreviewParticle)
    {
        State->PreviewParticle->ParticleSystem.AddEmitter(NewObject<UParticleEmitter>());
    }
}
void SParticleEditWindow::AddSpawnModule(const FString& ClassName)
{
    if (State->SelectedEmitter)
    {
        UParticleLODLevel* LOD = State->SelectedEmitter->GetParticleLODLevelWithIndex(0);
        UObject* obj = NewObject(UClass::FindClass(ClassName));
        if (UParticleModule* Module = Cast<UParticleModule>(obj))
        {
            LOD->AddSpawnModule(Module);
        }
    }
}
void SParticleEditWindow::AddUpdateModule(const FString& ClassName)
{

    if (State->SelectedEmitter)
    {
        UParticleLODLevel* LOD = State->SelectedEmitter->GetParticleLODLevelWithIndex(0);
        UObject* obj = NewObject(UClass::FindClass(ClassName));
        if (UParticleModule* Module = Cast<UParticleModule>(obj))
        {
            LOD->AddUpdateModule(Module);
        }
    }
}

void SParticleEditWindow::CreateParticleEditor(const FString& Path)
{
    std::filesystem::path FolderPath = GContentDir + "/Resources/Particle";
    UParticleAsset* ParticleAsset = UParticleAsset::Create(FolderPath.string());
    ParticleAsset->ParticleSystem.AddEmitter(NewObject<UParticleEmitter>());
    ParticleAsset->ParticleSystem.AddEmitter(NewObject<UParticleEmitter>());
    ParticleAsset->ParticleSystem.Emitters[1]->SetMaxParticleCount(3);
    ParticleAsset->Save();

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
    CenterRect = Rect;
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
    State->PreviewParticle = UResourceManager::GetInstance().Load<UParticleAsset>(AssetPath);
}
void SParticleEditWindow::OnRender()
{
    HoveredWindow = EParticleEditorWindow::None;
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
        UParticleSystem& CurParticleSystem = State->PreviewParticle->ParticleSystem;

        if (ImGui::Button("Save"))
        {

        }
        ImGui::SameLine();
        if (ImGui::Button("Restart"))
        {

        }


        ImVec2 ChildSize = Size * 0.5f;
        ImGui::BeginChild("Viewport", ChildSize);
        ImGui::Text("Viewport");
        if (ImGui::IsWindowHovered()) 
        {
            HoveredWindow = EParticleEditorWindow::Viewport;
        }
        //World Rendering
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("Emitter", ChildSize);
        ImGui::Text("Emitter");
        if (ImGui::IsWindowHovered())
        {
            HoveredWindow = EParticleEditorWindow::Emitter;
        }
        DrawEmitterDropdown();
        //
        ImGui::EndChild();

        ImGui::BeginChild("Detail", ChildSize);
        if (ImGui::IsWindowHovered())
        {
            HoveredWindow = EParticleEditorWindow::Detail;
        }

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

        UE_LOG("%d", Button);

    }
}

void SParticleEditWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (!State || !State->Viewport) return;

    UE_LOG("%d", Button);
    switch (HoveredWindow)
    {
    case EParticleEditorWindow::Viewport:
        bEmitterDropdown = false;
        break;
    case EParticleEditorWindow::Emitter:
        if (Button == 0) //left click
        {
            
        }
        else
        {
            bEmitterDropdown = true;   
            EmitterDropdownPos = ImGui::GetMousePos();
        }
        break;
    case EParticleEditorWindow::Detail:
        bEmitterDropdown = false;
        break;
    case EParticleEditorWindow::None:
        bEmitterDropdown = false;
        break;
    }
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


void SParticleEditWindow::DrawEmitterDropdown()
{
    if (bEmitterDropdown == false)
    {
        return;
    }

    ImGui::OpenPopup("EmitterDropdown");

    ImGui::SetNextWindowPos(EmitterDropdownPos);
    if (ImGui::BeginPopup("EmitterDropdown"))
    {
        for (auto& pair : DropdownActionMap)
        {
            const FString& Category = pair.first;
            const TMap<FString, FMenuAction>& SubCategoryMap = pair.second;
            if (ImGui::Selectable(Category.c_str(), false, ImGuiSelectableFlags_DontClosePopups))
            {
                
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
            {
                ImVec2 SubCategoryPos;
                SubCategoryPos.x = ImGui::GetItemRectMax().x;
                SubCategoryPos.y = ImGui::GetItemRectMin().y;
                ImGui::SetNextWindowPos(SubCategoryPos);
                ImGui::OpenPopup(Category.c_str());
            }
            if (ImGui::BeginPopup(Category.c_str()))
            {
                for (auto& SubPair : SubCategoryMap)
                {
                    const FString& SubCategory = SubPair.first;
                    const FMenuAction& SubCategorySelectAction = SubPair.second;
                    if (ImGui::Selectable(SubCategory.c_str()))
                    {
                        SubCategorySelectAction.Action(this);
                    }
                }
                ImGui::EndPopup();
            }
        }
        ImGui::EndPopup();
    }
    
}
