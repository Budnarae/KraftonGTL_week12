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
#include "Source/Runtime/Engine/Particle/ParticleModuleColorOverLife.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleLifeTime.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleLocation.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleSize.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleVelocity.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleSpawn.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleCollision.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleSizeOverLife.h"
#include "Source/Runtime/Engine/Particle/ParticleSystemComponent.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleBeamTarget.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleBeamNoise.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleBeamWidth.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleBeamColorOverLength.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleRibbonWidth.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleRibbonColorOverLength.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleTypeDataBase.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleTypeDataBeam.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleTypeDataRibbon.h"
#include "Source/Runtime/Engine/Particle/ParticleModuleTypeDataMesh.h"
#include "Source/Runtime/Engine/Particle/ParticleVariable.h"
#include "Source/Runtime/Engine/Particle/ParticleLODLevel.h"

ImVec2 TopMenuBarOffset = ImVec2(0, 30);
ImVec2 TopMenuBarSize = ImVec2(-1, 40);

ImVec4 HoveredColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); //마우스 올리면
ImVec4 ActiveColor = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); //선택했을때 클릭

ImVec4 EmitterColor = ImVec4(0.25f, 0.25f, 0.8f, 1.0f);//선택된
ImVec4 ModuleColor = ImVec4(0.25f, 0.8f, 0.25f, 1.0f);
ImVec4 TypeDataColor = ImVec4(0.8f, 0.25f, 0.8f, 1.0f);//TypeData 모듈

ImVec2 EmitterSize = ImVec2(100, 40);
ImVec2 RequireModuleSize = ImVec2(100, 30);
ImVec2 ModuleSize = ImVec2(100, 20);

//Dropdown에서 첫 카테고리안에 두번째 생성할 목록이 들어가 있다.
//각 생성할 목록들을 클릭하면 그 값이 현재 클릭된 이미터를 기준으로 추가된다.
//즉 파티클 시스템에 추가 함수가 있고, 각 목록들은 그함수에 연결되어 있다.
//함수 연결? 아니면 string?
//함수연결이 낫겠지

FRect GetWindowRect()
{
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    FRect Rect;
    Rect.Left = pos.x; Rect.Top = pos.y; Rect.Right = pos.x + size.x; Rect.Bottom = pos.y + size.y; Rect.UpdateMinMax();
    return Rect;
}

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
    case EMenuActionType::SetTypeData:
        ParticleEditWindow->SetTypeDataModule(TypeDataType);
        break;
    case EMenuActionType::RemoveTypeData:
        ParticleEditWindow->RemoveTypeDataModule();
        break;
    }
}

TMap<FString, TMap<FString, FMenuAction>> DropdownActionMap =
{
    {"파티클 시스템",
    {
        {"이미터 추가", FMenuAction::CreateAddEmitter(1)},
    }
    },
    {"타입",
    {
        {"기본 (스프라이트)", FMenuAction::CreateRemoveTypeData()},
        {"메시", FMenuAction::CreateSetTypeData(EDET_Mesh)},
        {"빔", FMenuAction::CreateSetTypeData(EDET_Beam)},
        {"리본", FMenuAction::CreateSetTypeData(EDET_Ribbon)},
    }
    },
    {"컬러",
    {
        {"초기컬러", FMenuAction::CreateSpawnModule(UParticleModuleColor::StaticClass()->Name)},
        {"컬러 오버 라이프", FMenuAction::CreateUpdateModule(UParticleModuleColorOverLife::StaticClass()->Name)},
    }
    },
    {"수명",
    {
        {"수명", FMenuAction::CreateSpawnModule(UParticleModuleLifetime::StaticClass()->Name)},
    }
    },
    {"위치",
    {
        {"초기 위치", FMenuAction::CreateSpawnModule(UParticleModuleLocation::StaticClass()->Name)},
    }
    },
    {"크기",
    {
        {"초기 크기", FMenuAction::CreateSpawnModule(UParticleModuleSize::StaticClass()->Name)},
        {"사이즈 오버 라이프", FMenuAction::CreateUpdateModule(UParticleModuleSizeOverLife::StaticClass()->Name)},
    }
    },
    {"속도",
    {
        {"초기 속도", FMenuAction::CreateSpawnModule(UParticleModuleVelocity::StaticClass()->Name)},
    }
    },
    {"스폰",
    {
        {"스폰", FMenuAction::CreateSpawnModule(UParticleModuleSpawn::StaticClass()->Name)},
    }
    },
    {"충돌",
    {
        {"콜리전", FMenuAction::CreateUpdateModule(UParticleModuleCollision::StaticClass()->Name)},
    }
    },
    {"빔",
    {
        {"빔 타겟", FMenuAction::CreateSpawnModule(UParticleModuleBeamTarget::StaticClass()->Name)},
        {"빔 노이즈", FMenuAction::CreateSpawnModule(UParticleModuleBeamNoise::StaticClass()->Name)},
        {"빔 너비", FMenuAction::CreateSpawnModule(UParticleModuleBeamWidth::StaticClass()->Name)},
        {"빔 길이별 색상", FMenuAction::CreateSpawnModule(UParticleModuleBeamColorOverLength::StaticClass()->Name)},
    }
    },
    {"리본",
    {
        {"리본 너비", FMenuAction::CreateSpawnModule(UParticleModuleRibbonWidth::StaticClass()->Name)},
        {"리본 길이별 색상", FMenuAction::CreateSpawnModule(UParticleModuleRibbonColorOverLength::StaticClass()->Name)},
    }
    },
    {"타입 데이터",
    {
        {"스프라이트 (기본)", FMenuAction::CreateRemoveTypeData()},  // TypeData 제거 = Sprite (default)
        {"메시", FMenuAction::CreateSetTypeData(EDET_Mesh)},
        {"빔", FMenuAction::CreateSetTypeData(EDET_Beam)},
        {"리본", FMenuAction::CreateSetTypeData(EDET_Ribbon)},
    }
    },
};


void SParticleEditWindow::AddEmitter(const int EmitterOffset)
{
    if (State->GetCachedParticle())
    {
        State->GetCachedParticle()->AddEmitter(NewObject<UParticleEmitter>());
        State->ReStartParticle();
    }
}
void SParticleEditWindow::AddSpawnModule(const FString& ClassName)
{
    UE_LOG("[AddSpawnModule] ClassName: %s", ClassName.c_str());

    if (!State->SelectedEmitter)
    {
        UE_LOG("[AddSpawnModule] ERROR: No emitter selected!");
        return;
    }

    UParticleLODLevel* LOD = State->SelectedEmitter->GetParticleLODLevelWithIndex(State->GetSelectedLODLevel());
    if (!LOD)
    {
        UE_LOG("[AddSpawnModule] ERROR: LOD is null!");
        return;
    }

    UClass* FoundClass = UClass::FindClass(ClassName);
    if (!FoundClass)
    {
        UE_LOG("[AddSpawnModule] ERROR: Class not found: %s", ClassName.c_str());
        return;
    }

    UObject* obj = NewObject(FoundClass);
    if (!obj)
    {
        UE_LOG("[AddSpawnModule] ERROR: Failed to create object!");
        return;
    }

    UParticleModule* Module = Cast<UParticleModule>(obj);
    if (!Module)
    {
        UE_LOG("[AddSpawnModule] ERROR: Cast to UParticleModule failed!");
        return;
    }

    LOD->AddSpawnModule(Module);
    UE_LOG("[AddSpawnModule] SUCCESS: Added module %s, SpawnModules count: %d",
           ClassName.c_str(), LOD->GetSpawnModule().Num());
}
void SParticleEditWindow::AddUpdateModule(const FString& ClassName)
{
    if (State->SelectedEmitter)
    {
        UParticleLODLevel* LOD = State->SelectedEmitter->GetParticleLODLevelWithIndex(State->GetSelectedLODLevel());
        UObject* obj = NewObject(UClass::FindClass(ClassName));
        if (UParticleModule* Module = Cast<UParticleModule>(obj))
        {
            LOD->AddUpdateModule(Module);
        }
        State->ReStartParticle();
    }
}

void SParticleEditWindow::SetTypeDataModule(int32 TypeDataType)
{
    if (!State->SelectedEmitter)
    {
        UE_LOG("[SParticleEditWindow] No emitter selected");
        return;
    }

    UParticleLODLevel* LOD = State->SelectedEmitter->GetParticleLODLevelWithIndex(State->GetSelectedLODLevel());
    if (!LOD)
    {
        UE_LOG("[SParticleEditWindow] LODLevel not found");
        return;
    }

    // 기존 TypeData 제거
    LOD->RemoveTypeDataModule();

    // 새로운 TypeData 생성
    UParticleModuleTypeDataBase* NewTypeData = nullptr;

    switch (TypeDataType)
    {
    case EDET_Mesh:
        NewTypeData = NewObject<UParticleModuleTypeDataMesh>();
        UE_LOG("[SParticleEditWindow] Created Mesh TypeData");
        break;
    case EDET_Beam:
        NewTypeData = NewObject<UParticleModuleTypeDataBeam>();
        UE_LOG("[SParticleEditWindow] Created Beam TypeData");
        break;
    case EDET_Ribbon:
        NewTypeData = NewObject<UParticleModuleTypeDataRibbon>();
        UE_LOG("[SParticleEditWindow] Created Ribbon TypeData");
        break;
    default:
        UE_LOG("[SParticleEditWindow] Unknown TypeData type: %d", TypeDataType);
        break;
    }

    if (NewTypeData)
    {
        LOD->SetTypeDataModule(NewTypeData);
    }
}

void SParticleEditWindow::RemoveTypeDataModule()
{
    if (!State->SelectedEmitter)
    {
        UE_LOG("[SParticleEditWindow] No emitter selected");
        return;
    }

    UParticleLODLevel* LOD = State->SelectedEmitter->GetParticleLODLevelWithIndex(State->GetSelectedLODLevel());
    if (!LOD)
    {
        UE_LOG("[SParticleEditWindow] LODLevel not found");
        return;
    }

    LOD->RemoveTypeDataModule();

    // TypeData 모듈이 선택된 상태였다면 선택 해제
    if (State->SelectedModule && Cast<UParticleModuleTypeDataBase>(State->SelectedModule))
    {
        State->SelectedModule = nullptr;
    }

    UE_LOG("[SParticleEditWindow] TypeData removed (reverted to Sprite)");
}

void SParticleEditWindow::RemoveEmitter()
{
    if (State->SelectedEmitter)
    {
        State->GetCachedParticle()->RemoveEmitter(State->SelectedEmitter);
        State->SelectedEmitter = nullptr;
        State->SelectedModule = nullptr;
        State->ReStartParticle();
    }
}
void SParticleEditWindow::RemoveModule()
{
    if (State->SelectedEmitter && State->SelectedModule)
    {
        UParticleLODLevel* LOD = State->SelectedEmitter->GetParticleLODLevelWithIndex(State->GetSelectedLODLevel());
        if (LOD->RemoveSpawnModule(State->SelectedModule) == false)
        {
            State->SelectedModule = nullptr;
            return;
        }
        if (LOD->RemoveUpdateModule(State->SelectedModule) == false)
        {
            State->SelectedModule = nullptr;
            return;
        }
        State->ReStartParticle();
    }
}
void SParticleEditWindow::ResetModule()
{

}

void SParticleEditWindow::ReStart()
{
    // 파티클 시스템 재활성화 (TypeData 변경 등 적용)
    if (State && State->ParticleActor)
    {
        UParticleSystemComponent* PSC = State->ParticleActor->GetParticleSystemComponent();
        if (PSC)
        {
            PSC->Activate(true);  // bReset = true로 완전 재시작
        }
    }
}

void SParticleEditWindow::Save()
{
    // TODO: 저장 로직 구현
}





void SParticleEditWindow::CreateParticleEditor(const FString& Path)
{
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

}

SParticleEditWindow::~SParticleEditWindow()
{
    if (State)
    {
        ParticleViewerBootstrap::DestroyViewerState(State);
        State = nullptr;
    }
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
    if (AssetPath.empty() == false) 
    {
        State->LoadCachedParticle(AssetPath);
    }
    
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
    HoveredWindowType = EHoveredWindowType::None;

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
        Rect = GetWindowRect();

        //상단 버튼 및 경로
        ImGui::Text("Path %s", State->GetPathConstChar());
        if (ImGui::Button("New"))
        {
            UParticleAsset* Asset = UParticleAsset::CreateAutoName(UParticleAsset::FolderPath.string());
            State->LoadCachedParticle(Asset->GetFilePath());
        }
        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            //현재 캐쉬를 파일에 저장하고 파일을 다시 읽어옴
            if (State->GetCachedParticle()) 
            {
                UParticleAsset::Save(State->ParticlePath, State->GetCachedParticle());
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load"))
        {
            FWideString FilePath = UResourceBase::OpenFileDialogGetPath(UParticleAsset::FolderPath, UParticleAsset::Extension, UParticleAsset::Desc);
            if (FilePath.empty() == false)
            {
                State->LoadCachedParticle(WideToUTF8(FilePath));
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Restart"))
        {
            State->ReStartParticle();
        }


        //뷰포트
        ImVec2 ChildSize = Size * 0.5f;
        ImGui::BeginChild("Viewport", ChildSize);
        ViewportRect = GetWindowRect();

        if (ImGui::IsWindowHovered())
        {
            HoveredWindowType = EHoveredWindowType::Viewport;
        }
        ImGui::Text("Viewport");
        ImGui::EndChild();
        ImGui::SameLine();

        //이미터
        ImGui::BeginChild("Emitter", ChildSize,0, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::IsWindowHovered())
        {
            HoveredWindowType = EHoveredWindowType::Emitter;
        }
        ImGui::Text("Emitter");

        DrawEmitterView();
        DrawEmitterDropdown();
        DrawModuleDropdown();
        DrawTypeDataDropdown();
        ImGui::EndChild();

        //디테일
        ImGui::BeginChild("Detail", ChildSize);
        if (ImGui::IsWindowHovered())
        {
            HoveredWindowType = EHoveredWindowType::Detail;
        }
        ImGui::Text("Detail");
        if (State->SelectedEmitter)
        {
            static char buf[128] = "";
            const FString& Name = State->SelectedEmitter->GetEmitterName();
            strncpy_s(buf, Name.c_str(), sizeof(buf));
            if (ImGui::InputText("EmitterName", buf, sizeof(buf)))
            {
                State->SelectedEmitter->SetEmitterName(buf);
            }
        }

        bool bChanged = false;
        if (State->SelectedModule)
        {
            bChanged |= UPropertyRenderer::RenderAllProperties(State->SelectedModule);
        }
        else
        {
            if (State->SelectedEmitter)
            {
                bChanged |= UPropertyRenderer::RenderAllProperties(State->SelectedEmitter);
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();


        //커브 미구현
        ImGui::BeginChild("CurveEditor", ChildSize);
        ImGui::EndChild();

        if (bChanged)
        {
            State->ReStartParticle();
        }
    }
    // ImGui::Begin()이 false를 반환해도 반드시 End()를 호출해야 함
    ImGui::End();
}
void SParticleEditWindow::OnUpdate(float DeltaSeconds)
{
    if (!State || !State->Viewport)
        return;
    if (State->World)
    {
        State->World->Tick(DeltaSeconds);
    }
    if (State && State->Client)
    {
        State->Client->Tick(DeltaSeconds);
    }
}

void SParticleEditWindow::OnMouseMove(FVector2D MousePos)
{
    if (!State || !State->Viewport) return;

    if (ViewportRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(ViewportRect.Left, ViewportRect.Top);
        State->Viewport->ProcessMouseMove((int32)LocalPos.X, (int32)LocalPos.Y);
    }
}

void SParticleEditWindow::OnMouseDown(FVector2D MousePos, uint32 Button)
{
    if (!State || !State->Viewport) return;



    if (ViewportRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(ViewportRect.Left, ViewportRect.Top);

        // First, always try gizmo picking (pass to viewport)
        State->Viewport->ProcessMouseButtonDown((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
}

void SParticleEditWindow::OnMouseUp(FVector2D MousePos, uint32 Button)
{
    if (!State || !State->Viewport) return;
    if (ViewportRect.Contains(MousePos))
    {
        FVector2D LocalPos = MousePos - FVector2D(ViewportRect.Left, ViewportRect.Top);
        State->Viewport->ProcessMouseButtonUp((int32)LocalPos.X, (int32)LocalPos.Y, (int32)Button);
    }
    switch (HoveredWindowType)
    {
    case EHoveredWindowType::Viewport:
        bEmitterDropdown = false;
        bModuleDropdown = false;
        bTypeDataDropdown = false;
        break;
    case EHoveredWindowType::Emitter:
        if (Button == 1 && ImGui::IsAnyItemHovered() == false)
        {
            bEmitterDropdown = true;
            EmitterDropdownPos = ImGui::GetMousePos();
        }
        break;
    case EHoveredWindowType::Detail:
        bEmitterDropdown = false;
        bModuleDropdown = false;
        bTypeDataDropdown = false;
        break;
    default:
        bEmitterDropdown = false;
        bModuleDropdown = false;
        bTypeDataDropdown = false;
        break;
    }
}

void SParticleEditWindow::OnRenderViewport()
{
    if (State && State->Viewport && ViewportRect.GetWidth() > 0 && ViewportRect.GetHeight() > 0)
    {
        const uint32 NewStartX = static_cast<uint32>(ViewportRect.Left);
        const uint32 NewStartY = static_cast<uint32>(ViewportRect.Top);
        const uint32 NewWidth = static_cast<uint32>(ViewportRect.Right - ViewportRect.Left);
        const uint32 NewHeight = static_cast<uint32>(ViewportRect.Bottom - ViewportRect.Top);
        State->Viewport->Resize(NewStartX, NewStartY, NewWidth, NewHeight);

        // 뷰포트 렌더링 (ImGui보다 먼저)
        State->Viewport->Render();
    }
}

//window우클릭
//module 우클릭
//void SParticleEditWindow::DrawModule(UParticleEmitter* ParentEmitter, UParticleModule* Module)
//{
//    if (Module == nullptr)
//    {
//        return;
//    }
//    FString RequireModuleGUIID = GetUniqueGUIIDWithPointer(Module->GetClass()->DisplayName, Module);
//    bool bSelected = Module == State->SelectedModule;
//    if(ImGui::T)
//}
void SParticleEditWindow::DrawModuleInEmitterView(UParticleEmitter* ParentEmitter, UParticleModule* Module, const ImVec2& Size)
{
    if (Module == nullptr)
    {
        return;
    }
    //FString RequireModuleGUIID = GetUniqueGUIIDWithPointer(Module->GetClass()->DisplayName, Module);
    ImGui::PushID(reinterpret_cast<uintptr_t>(Module));
    bool bSelected = Module == State->SelectedModule;
    if (ImGui::Selectable(Module->GetClass()->DisplayName, bSelected, 0, Size))
    {
        State->SelectedModule = Module;
        State->SelectedEmitter = ParentEmitter;
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        State->SelectedModule = Module;
        State->SelectedEmitter = ParentEmitter;

        // TypeData 모듈인지 확인
        if (Cast<UParticleModuleTypeDataBase>(Module))
        {
            bTypeDataDropdown = true;
            TypeDataDropdownPos = ImGui::GetMousePos();
        }
        else
        {
            bModuleDropdown = true;
            ModuleDropdownPos = ImGui::GetMousePos();
        }
    }
    ImGui::SameLine();
    bool bActvie = Module->GetActive();
    if (ImGui::Checkbox("##", &bActvie))
    {
        Module->SetActive(bActvie);
        State->ReStartParticle();
    }
    ImGui::PopID();

}

void SParticleEditWindow::DrawLODSelector()
{
    if (!State->GetCachedParticle())
    {
        return;
    }

    ImGui::Separator();
    ImGui::Text("LOD 레벨");
    ImGui::SameLine();

    // LOD 레벨 선택 (< 버튼, 숫자, > 버튼)
    int32 CurrentLOD = State->GetSelectedLODLevel();

    if (ImGui::Button("<##LOD"))
    {
        if (CurrentLOD > 0)
        {
            State->SetSelectedLODLevel(CurrentLOD - 1);
        }
    }
    ImGui::SameLine();
    ImGui::Text("%d", CurrentLOD);
    ImGui::SameLine();
    if (ImGui::Button(">##LOD"))
    {
        if (CurrentLOD < MAX_PARTICLE_LODLEVEL - 1)
        {
            State->SetSelectedLODLevel(CurrentLOD + 1);
        }
    }

    // 현재 LOD의 거리 설정 (선택된 에미터가 있을 때만)
    if (State->SelectedEmitter)
    {
        UParticleLODLevel* LODLevel = State->SelectedEmitter->GetParticleLODLevelWithIndex(CurrentLOD);
        if (LODLevel)
        {
            ImGui::SameLine();
            float LODDistance = LODLevel->GetLODDistance();
            ImGui::SetNextItemWidth(80);
            if (ImGui::DragFloat("거리##LODDist", &LODDistance, 10.0f, 0.0f, 100000.0f, "%.0f"))
            {
                LODLevel->SetLODDistance(LODDistance);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("이 LOD가 활성화되는 최소 거리\nLOD 0은 항상 0.0");
            }
        }
    }

    ImGui::Separator();
}

void SParticleEditWindow::DrawEmitterView()
{
    if (State->GetCachedParticle() == nullptr)
    {
        return;
    }

    // LOD 선택 UI 먼저 렌더링
    DrawLODSelector();

    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ActiveColor);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HoveredColor);
    UParticleSystem* CurParticle = State->GetCachedParticle();
    TArray<UParticleEmitter*> Emitters = CurParticle->GetEmitters();
    int32 SelectedLOD = State->GetSelectedLODLevel();
    for (UParticleEmitter* Emitter : Emitters)
    {
        UParticleLODLevel* ParticleLOD = Emitter->GetParticleLODLevelWithIndex(SelectedLOD);
        UParticleModuleTypeDataBase* TypeDataModule = ParticleLOD->GetTypeDataModule();
        UParticleModule* RequireModule = ParticleLOD->GetRequiredModule();
        TArray<UParticleModule*>& SpawnModules = ParticleLOD->GetSpawnModule();
        TArray<UParticleModule*>& UpdateModules = ParticleLOD->GetUpdateModule();

        ImGui::BeginGroup();
        ImGui::PushID(reinterpret_cast<uintptr_t>(Emitter));
        FString GUIID = GetUniqueGUIIDWithPointer(Emitter->GetEmitterName(), Emitter);
        bool bSelected = Emitter == State->SelectedEmitter;
        ImGui::PushStyleColor(ImGuiCol_Header, EmitterColor);

        if (ImGui::Selectable(GUIID.c_str(), bSelected, 0, EmitterSize))
        {
            //선택된 이미터 이걸로 변경
            State->SelectedEmitter = Emitter;
            State->SelectedModule = nullptr;
        }
        ImGui::SameLine();
        bool bActvie = Emitter->GetActive();
        if (ImGui::Checkbox("##", &bActvie))
        {
            Emitter->SetActive(bActvie);
            State->ReStartParticle();
        }
        ImGui::PopStyleColor();

        // TypeData 모듈 렌더링 (RequiredModule 위에 표시)
        ImGui::PushStyleColor(ImGuiCol_Header, TypeDataColor);
        if (TypeDataModule)
        {
            // TypeData가 있으면 실제 타입 표시
            DrawModuleInEmitterView(Emitter, TypeDataModule, RequireModuleSize);
        }
        else
        {
            // TypeData가 없으면 기본 스프라이트 표시 (선택 불가능)
            FString DefaultLabel = GetUniqueGUIIDWithPointer("타입: 스프라이트 (기본)", Emitter);
            ImGui::Selectable(DefaultLabel.c_str(), false, ImGuiSelectableFlags_Disabled, RequireModuleSize);
        }
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Header, ModuleColor);


        DrawModuleInEmitterView(Emitter, RequireModule, RequireModuleSize);

        for (UParticleModule* Module : SpawnModules)
        {
            DrawModuleInEmitterView(Emitter, Module, ModuleSize);
        }
        for (UParticleModule* Module : UpdateModules)
        {
            DrawModuleInEmitterView(Emitter, Module, ModuleSize);
        }

        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    ImGui::PopStyleColor(2);

}

void SParticleEditWindow::DrawModuleDropdown()
{
    if (bModuleDropdown == false)
    {
        return;
    }
    ImGui::OpenPopup("ModuleDropdown");

    ImGui::SetNextWindowPos(ModuleDropdownPos);
    if (ImGui::BeginPopup("ModuleDropdown"))
    {
        if (ImGui::Selectable("모듈 새로고침"))
        {
            ResetModule();
        }
        if (ImGui::Selectable("모듈 제거"))
        {
            RemoveModule();
        }
        ImGui::EndPopup();
    }
}

void SParticleEditWindow::DrawTypeDataDropdown()
{
    if (!bTypeDataDropdown)
    {
        return;
    }

    ImGui::OpenPopup("TypeDataDropdown");
    ImGui::SetNextWindowPos(TypeDataDropdownPos);

    if (ImGui::BeginPopup("TypeDataDropdown"))
    {
        if (ImGui::BeginMenu("타입 변경"))
        {
            if (ImGui::Selectable("기본 (스프라이트)"))
            {
                RemoveTypeDataModule();
                bTypeDataDropdown = false;
            }
            if (ImGui::Selectable("메시"))
            {
                SetTypeDataModule(EDET_Mesh);
                bTypeDataDropdown = false;
            }
            if (ImGui::Selectable("빔"))
            {
                SetTypeDataModule(EDET_Beam);
                bTypeDataDropdown = false;
            }
            if (ImGui::Selectable("리본"))
            {
                SetTypeDataModule(EDET_Ribbon);
                bTypeDataDropdown = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::Selectable("타입 제거"))
        {
            RemoveTypeDataModule();
            bTypeDataDropdown = false;
        }

        ImGui::EndPopup();
    }
    else
    {
        bTypeDataDropdown = false;
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
