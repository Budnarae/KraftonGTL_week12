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
#include "Source/Runtime/Core/Math/Statistics.h"
#include "Source/Runtime/Core/Object/Property.h"

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

    // TypeData 변경 후 EmitterInstances를 재초기화해야 새 EmitterType이 적용됨
    State->ReStartParticle();
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

    // TypeData 제거 후 EmitterInstances를 재초기화해야 Sprite로 복귀됨
    State->ReStartParticle();
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
        UParticleLODLevel* LOD = State->SelectedEmitter->GetParticleLODLevelWithIndex(0);

        UParticleModuleRequired* Required = Cast<UParticleModuleRequired>(State->SelectedModule);
        UParticleModuleSpawn* Spawn = Cast<UParticleModuleSpawn>(State->SelectedModule);

        int32 SpawnModuleCount = 0;
        for (UParticleModule* Module : LOD->GetSpawnModule())
        {
            UParticleModuleSpawn* SpawnModules = Cast<UParticleModuleSpawn>(Module);
            if (SpawnModules) SpawnModuleCount++;
        }
            
        if (Required || (Spawn && SpawnModuleCount == 1))
        {
            UE_LOG("[SParticleEditWindow] You can't remove that module because that module is necessary.");
            return;
        }

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
            // 새 에셋 생성 후 캐시 클리어 (Template 드롭다운에 반영)
            UPropertyRenderer::ClearResourcesCache();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            //현재 캐쉬를 파일에 저장하고 파일을 다시 읽어옴
            if (State->GetCachedParticle())
            {
                UParticleAsset::Save(State->ParticlePath, State->GetCachedParticle());
                // 저장 후 캐시 클리어 (Template 드롭다운에 반영)
                UPropertyRenderer::ClearResourcesCache();
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


        //커브 편집기
        ImGui::BeginChild("CurveEditor", ChildSize);
        if (ImGui::IsWindowHovered())
        {
            HoveredWindowType = EHoveredWindowType::Detail;
        }
        ImGui::Text("Curve Editor");
        ImGui::Separator();

        // 선택된 모듈의 Curve 모드 프로퍼티 표시
        UObject* TargetObject = State->SelectedModule ? (UObject*)State->SelectedModule : (UObject*)State->SelectedEmitter;
        if (TargetObject)
        {
            UClass* Class = TargetObject->GetClass();
            const TArray<FProperty>& Properties = Class->GetProperties();

            bool bHasCurveProperty = false;

            // Curve 모드인 Distribution 프로퍼티 찾아서 그래픽으로 표시
            for (const FProperty& Prop : Properties)
            {
                // FRawDistribution<float> 타입 체크
                if (Prop.Type == EPropertyType::RawDistribution_Float)
                {
                    FRawDistribution<float>* Value = Prop.GetValuePtr<FRawDistribution<float>>(TargetObject);
                    if (Value && Value->Mode == EDistributionMode::Curve)
                    {
                        bHasCurveProperty = true;

                        // 커브 이름 표시
                        ImGui::PushID(Prop.Name);
                        if (ImGui::CollapsingHeader(Prop.Name, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            // 그래프 영역 (축 라벨 공간 확보)
                            const float AxisLabelHeight = 20.0f;
                            const float AxisLabelWidth = 40.0f;
                            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
                            canvas_pos.x += AxisLabelWidth;
                            canvas_pos.y += 10.0f;
                            ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x - AxisLabelWidth - 10.0f, 220.0f);
                            ImDrawList* draw_list = ImGui::GetWindowDrawList();

                            // 실제 데이터 범위 계산
                            float calcMinVal = FLT_MAX;
                            float calcMaxVal = -FLT_MAX;
                            for (const auto& Point : Value->Curve.Points)
                            {
                                if (std::isfinite(Point.OutVal))
                                {
                                    calcMinVal = FMath::Min(calcMinVal, Point.OutVal);
                                    calcMaxVal = FMath::Max(calcMaxVal, Point.OutVal);
                                }
                            }

                            // 기본값 설정
                            if (calcMinVal == FLT_MAX || !std::isfinite(calcMinVal)) calcMinVal = 0.0f;
                            if (calcMaxVal == -FLT_MAX || !std::isfinite(calcMaxVal)) calcMaxVal = 1.0f;

                            // 데이터 범위 (최소 1.0 확보)
                            float dataRange = FMath::Max(calcMaxVal - calcMinVal, 1.0f);
                            float dataMinVal = calcMinVal;

                            // 표시용 범위 (패딩 20% 추가)
                            float padding = dataRange * 0.2f;
                            float displayMinVal = dataMinVal - padding;
                            float displayMaxVal = calcMaxVal + padding;
                            float displayRange = displayMaxVal - displayMinVal;

                            // 범위 안전성 검사
                            if (!std::isfinite(displayMinVal) || !std::isfinite(displayMaxVal) || !std::isfinite(displayRange))
                            {
                                displayMinVal = 0.0f;
                                displayMaxVal = 1.0f;
                                displayRange = 1.0f;
                            }

                            // 상태 변수 먼저 로드 (배경/커브 그리기 전에 필요)
                            ImGuiStorage* storage = ImGui::GetStateStorage();
                            int state_key_selected = ImGui::GetID("selected");
                            int state_key_dragging = ImGui::GetID("dragging");
                            int state_key_prev_mouse_x = ImGui::GetID("prev_mouse_x");
                            int state_key_prev_mouse_y = ImGui::GetID("prev_mouse_y");
                            int state_key_view_min_time = ImGui::GetID("view_min_time");
                            int state_key_view_max_time = ImGui::GetID("view_max_time");
                            int state_key_panning = ImGui::GetID("panning");
                            int state_key_pan_start_x = ImGui::GetID("pan_start_x");

                            int selected_point = storage->GetInt(state_key_selected, -1);
                            bool dragging = storage->GetBool(state_key_dragging, false);

                            // 줌/팬 상태 (기본값: MinTime~MaxTime 전체 범위)
                            float view_min_time = storage->GetFloat(state_key_view_min_time, Value->MinTime);
                            float view_max_time = storage->GetFloat(state_key_view_max_time, Value->MaxTime);
                            bool panning = storage->GetBool(state_key_panning, false);
                            float view_time_range = view_max_time - view_min_time;

                            // 배경
                            draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(30, 30, 35, 255));
                            draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(120, 120, 140, 255), 0.0f, 0, 1.5f);

                            // 그리드
                            for (int i = 0; i <= 10; ++i)
                            {
                                float alpha = (i == 0 || i == 10) ? 100.0f : 60.0f;
                                float x = canvas_pos.x + (canvas_size.x * i / 10.0f);
                                draw_list->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y), IM_COL32(70, 70, 80, (int)alpha));

                                float y = canvas_pos.y + (canvas_size.y * i / 10.0f);
                                draw_list->AddLine(ImVec2(canvas_pos.x, y), ImVec2(canvas_pos.x + canvas_size.x, y), IM_COL32(70, 70, 80, (int)alpha));
                            }

                            // 축 라벨 (줌/팬 반영)
                            char time_label[64];
                            sprintf_s(time_label, "Time (%.2f ~ %.2f)", view_min_time, view_max_time);
                            draw_list->AddText(ImVec2(canvas_pos.x + 10.0f, canvas_pos.y + canvas_size.y + 5.0f), IM_COL32(200, 200, 200, 255), time_label);

                            ImVec2 value_label_pos = ImVec2(canvas_pos.x - AxisLabelWidth + 5.0f, canvas_pos.y + canvas_size.y * 0.5f);
                            draw_list->AddText(value_label_pos, IM_COL32(200, 200, 200, 255), "Value");

                            // 축 값 표시
                            char buf[32];
                            sprintf_s(buf, "%.2f", displayMaxVal);
                            draw_list->AddText(ImVec2(canvas_pos.x - AxisLabelWidth + 5.0f, canvas_pos.y - 5.0f), IM_COL32(150, 150, 150, 255), buf);
                            sprintf_s(buf, "%.2f", displayMinVal);
                            draw_list->AddText(ImVec2(canvas_pos.x - AxisLabelWidth + 5.0f, canvas_pos.y + canvas_size.y - 10.0f), IM_COL32(150, 150, 150, 255), buf);

                            // 커브 선 그리기 (줌/팬 적용)
                            if (Value->Curve.Points.Num() > 1)
                            {
                                for (int i = 0; i < Value->Curve.Points.Num() - 1; ++i)
                                {
                                    const auto& p1 = Value->Curve.Points[i];
                                    const auto& p2 = Value->Curve.Points[i + 1];

                                    if (std::isfinite(p1.OutVal) && std::isfinite(p2.OutVal))
                                    {
                                        float t1 = (p1.InVal - view_min_time) / view_time_range;
                                        float t2 = (p2.InVal - view_min_time) / view_time_range;

                                        // 화면 범위 밖이면 클리핑
                                        if (t2 < 0.0f || t1 > 1.0f) continue;

                                        float x1 = canvas_pos.x + FMath::Clamp(t1, 0.0f, 1.0f) * canvas_size.x;
                                        float y1 = canvas_pos.y + canvas_size.y - FMath::Clamp((p1.OutVal - displayMinVal) / displayRange, 0.0f, 1.0f) * canvas_size.y;
                                        float x2 = canvas_pos.x + FMath::Clamp(t2, 0.0f, 1.0f) * canvas_size.x;
                                        float y2 = canvas_pos.y + canvas_size.y - FMath::Clamp((p2.OutVal - displayMinVal) / displayRange, 0.0f, 1.0f) * canvas_size.y;

                                        draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(100, 200, 255, 255), 2.5f);
                                    }
                                }
                            }

                            // 키프레임 점 그리기 및 인터랙션
                            ImGui::SetCursorScreenPos(canvas_pos);
                            ImGui::InvisibleButton("canvas", canvas_size);
                            bool is_hovered = ImGui::IsItemHovered();

                            // Ctrl + 마우스 휠로 줌 (마우스 위치 중심)
                            if (is_hovered && ImGui::GetIO().MouseWheel != 0.0f && ImGui::GetIO().KeyCtrl)
                            {
                                float wheel = ImGui::GetIO().MouseWheel;
                                float mouse_t = (ImGui::GetMousePos().x - canvas_pos.x) / canvas_size.x;
                                float zoom_t = view_min_time + mouse_t * (view_max_time - view_min_time);

                                float zoom_factor = wheel > 0 ? 0.9f : 1.1f;
                                float new_range = (view_max_time - view_min_time) * zoom_factor;

                                view_min_time = zoom_t - (zoom_t - view_min_time) * zoom_factor;
                                view_max_time = view_min_time + new_range;

                                // 범위 제한 (최소 0.01, MinTime~MaxTime 범위)
                                float min_range = 0.01f;
                                if (new_range < min_range) {
                                    view_min_time = zoom_t - min_range * 0.5f;
                                    view_max_time = zoom_t + min_range * 0.5f;
                                }
                                if (view_min_time < Value->MinTime) view_min_time = Value->MinTime;
                                if (view_max_time > Value->MaxTime) view_max_time = Value->MaxTime;

                                storage->SetFloat(state_key_view_min_time, view_min_time);
                                storage->SetFloat(state_key_view_max_time, view_max_time);
                            }

                            // 중간 버튼으로 팬
                            if (is_hovered && ImGui::IsMouseClicked(2))
                            {
                                panning = true;
                                storage->SetBool(state_key_panning, true);
                                storage->SetFloat(state_key_pan_start_x, ImGui::GetMousePos().x);
                            }
                            if (panning && ImGui::IsMouseDown(2))
                            {
                                float pan_start_x = storage->GetFloat(state_key_pan_start_x, ImGui::GetMousePos().x);
                                float delta_x = ImGui::GetMousePos().x - pan_start_x;
                                float delta_t = -(delta_x / canvas_size.x) * (view_max_time - view_min_time);

                                view_min_time += delta_t;
                                view_max_time += delta_t;

                                storage->SetFloat(state_key_view_min_time, view_min_time);
                                storage->SetFloat(state_key_view_max_time, view_max_time);
                                storage->SetFloat(state_key_pan_start_x, ImGui::GetMousePos().x);
                            }
                            if (ImGui::IsMouseReleased(2))
                            {
                                panning = false;
                                storage->SetBool(state_key_panning, false);
                            }

                            // 줌/팬 후 범위 재계산
                            view_time_range = view_max_time - view_min_time;

                            for (int i = 0; i < Value->Curve.Points.Num(); ++i)
                            {
                                auto& Point = Value->Curve.Points[i];
                                if (!std::isfinite(Point.OutVal)) continue;

                                // 시간축 좌표 변환 (줌/팬 적용)
                                float normalized_t = (Point.InVal - view_min_time) / view_time_range;
                                float x = canvas_pos.x + FMath::Clamp(normalized_t, 0.0f, 1.0f) * canvas_size.x;
                                float y = canvas_pos.y + canvas_size.y - FMath::Clamp((Point.OutVal - displayMinVal) / displayRange, 0.0f, 1.0f) * canvas_size.y;

                                // 화면 밖이면 스킵
                                if (normalized_t < 0.0f || normalized_t > 1.0f) continue;

                                bool is_point_hovered = ImGui::IsMouseHoveringRect(ImVec2(x - 6, y - 6), ImVec2(x + 6, y + 6));

                                // 마우스 클릭으로 선택
                                if (is_point_hovered && ImGui::IsMouseClicked(0))
                                {
                                    selected_point = i;
                                    dragging = true;
                                    storage->SetInt(state_key_selected, i);
                                    storage->SetBool(state_key_dragging, true);
                                    // 드래그 시작 시 마우스 위치 저장
                                    ImVec2 mouse_pos = ImGui::GetMousePos();
                                    storage->SetFloat(state_key_prev_mouse_x, mouse_pos.x);
                                    storage->SetFloat(state_key_prev_mouse_y, mouse_pos.y);
                                }

                                // 드래그 (변화량 기반)
                                if (dragging && selected_point == i && ImGui::IsMouseDown(0))
                                {
                                    ImVec2 mouse_pos = ImGui::GetMousePos();
                                    float prev_mouse_x = storage->GetFloat(state_key_prev_mouse_x, mouse_pos.x);
                                    float prev_mouse_y = storage->GetFloat(state_key_prev_mouse_y, mouse_pos.y);

                                    // 마우스 이동량 계산
                                    float delta_x = mouse_pos.x - prev_mouse_x;
                                    float delta_y = mouse_pos.y - prev_mouse_y;

                                    // 감도 설정 (1.0 = 정상 속도)
                                    const float sensitivity = 1.0f;

                                    // X축: Time (줌/팬 범위 고려)
                                    float time_delta = (delta_x / canvas_size.x) * view_time_range * sensitivity;
                                    Point.InVal += time_delta;

                                    // 시간 값을 MinTime~MaxTime 범위로 제한
                                    Point.InVal = FMath::Clamp(Point.InVal, Value->MinTime, Value->MaxTime);

                                    // Y축: Value
                                    float value_delta = -(delta_y / canvas_size.y) * displayRange * sensitivity;
                                    Point.OutVal += value_delta;

                                    // NaN/Inf 방지 및 최종 값 제한
                                    if (!std::isfinite(Point.OutVal)) Point.OutVal = 0.0f;
                                    Point.OutVal = FMath::Clamp(Point.OutVal, -100000.0f, 100000.0f);

                                    // 현재 마우스 위치 저장
                                    storage->SetFloat(state_key_prev_mouse_x, mouse_pos.x);
                                    storage->SetFloat(state_key_prev_mouse_y, mouse_pos.y);

                                    bChanged = true;
                                }

                                // 우클릭으로 삭제
                                if (is_point_hovered && ImGui::IsMouseClicked(1))
                                {
                                    Value->Curve.Points.RemoveAt(i);
                                    storage->SetInt(state_key_selected, -1);
                                    bChanged = true;
                                    break;
                                }

                                // 점 그리기
                                ImU32 color = (selected_point == i) ? IM_COL32(255, 255, 255, 255) : IM_COL32(100, 200, 255, 255);
                                draw_list->AddCircleFilled(ImVec2(x, y), 6.0f, color);
                                draw_list->AddCircle(ImVec2(x, y), 6.0f, IM_COL32(0, 0, 0, 255), 16, 2.0f);

                                // 호버 시 값 표시
                                if (is_point_hovered)
                                {
                                    char tooltip[64];
                                    sprintf_s(tooltip, "T: %.3f\nV: %.3f", Point.InVal, Point.OutVal);
                                    ImGui::SetTooltip("%s", tooltip);
                                }
                            }

                            if (ImGui::IsMouseReleased(0))
                            {
                                storage->SetBool(state_key_dragging, false);
                                // 정렬
                                if (selected_point != -1)
                                {
                                    std::sort(Value->Curve.Points.begin(), Value->Curve.Points.end(),
                                        [](const FInterpCurvePoint<float>& A, const FInterpCurvePoint<float>& B)
                                        {
                                            return A.InVal < B.InVal;
                                        });
                                }
                            }

                            // 더블클릭으로 키프레임 추가
                            if (is_hovered && ImGui::IsMouseDoubleClicked(0) && !panning)
                            {
                                ImVec2 mouse_pos = ImGui::GetMousePos();
                                float normalized_t = (mouse_pos.x - canvas_pos.x) / canvas_size.x;
                                float t = view_min_time + normalized_t * view_time_range;

                                // 시간 값을 MinTime~MaxTime 범위로 제한
                                t = FMath::Clamp(t, Value->MinTime, Value->MaxTime);

                                float normalized_y = 1.0f - (mouse_pos.y - canvas_pos.y) / canvas_size.y;
                                // normalized_y를 합리적인 범위로 제한
                                normalized_y = FMath::Clamp(normalized_y, -1.0f, 2.0f);
                                float v = displayMinVal + normalized_y * displayRange;
                                v = FMath::Clamp(v, -100000.0f, 100000.0f);

                                if (std::isfinite(v))
                                {
                                    Value->Curve.AddPoint(t, v);
                                    bChanged = true;
                                }
                            }

                            // 하단 여백 확보
                            ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x, canvas_pos.y + canvas_size.y + AxisLabelHeight + 5.0f));
                            ImGui::Dummy(ImVec2(0, 5));

                            // 인터랙션 가이드
                            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "LClick: Select | Drag: Move | RClick: Delete | DblClick: Add | Ctrl+MWheel: Zoom | MButton: Pan");
                            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "MWheel: Zoom | MButton: Pan");
                        }
                        ImGui::PopID();
                    }
                }
                // FRawDistribution<FVector> 타입 체크
                else if (Prop.Type == EPropertyType::RawDistribution_FVector)
                {
                    FRawDistribution<FVector>* Value = Prop.GetValuePtr<FRawDistribution<FVector>>(TargetObject);
                    if (Value && Value->Mode == EDistributionMode::Curve)
                    {
                        bHasCurveProperty = true;

                        // FVector 커브는 X, Y, Z 별도 표시
                        ImGui::PushID(Prop.Name);
                        if (ImGui::CollapsingHeader(Prop.Name, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            const char* AxisNames[] = { "X", "Y", "Z" };
                            ImU32 AxisColors[] = { IM_COL32(255, 80, 80, 255), IM_COL32(80, 255, 80, 255), IM_COL32(80, 180, 255, 255) };

                            for (int axis = 0; axis < 3; ++axis)
                            {
                                ImGui::PushID(axis);
                                if (ImGui::TreeNode(AxisNames[axis]))
                                {
                                    // 그래프 영역 (축 라벨 공간 확보)
                                    const float AxisLabelHeight = 20.0f;
                                    const float AxisLabelWidth = 40.0f;
                                    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
                                    canvas_pos.x += AxisLabelWidth;
                                    canvas_pos.y += 10.0f;
                                    ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x - AxisLabelWidth - 10.0f, 170.0f);
                                    ImDrawList* draw_list = ImGui::GetWindowDrawList();

                                    // 실제 데이터 범위 계산
                                    float calcMinVal = FLT_MAX;
                                    float calcMaxVal = -FLT_MAX;
                                    for (const auto& Point : Value->Curve.Points)
                                    {
                                        float val = (axis == 0) ? Point.OutVal.X : (axis == 1) ? Point.OutVal.Y : Point.OutVal.Z;
                                        if (std::isfinite(val))
                                        {
                                            calcMinVal = FMath::Min(calcMinVal, val);
                                            calcMaxVal = FMath::Max(calcMaxVal, val);
                                        }
                                    }

                                    // 기본값 설정
                                    if (calcMinVal == FLT_MAX || !std::isfinite(calcMinVal)) calcMinVal = 0.0f;
                                    if (calcMaxVal == -FLT_MAX || !std::isfinite(calcMaxVal)) calcMaxVal = 1.0f;

                                    // 데이터 범위 (최소 1.0 확보)
                                    float dataRange = FMath::Max(calcMaxVal - calcMinVal, 1.0f);
                                    float dataMinVal = calcMinVal;

                                    // 표시용 범위 (패딩 20% 추가)
                                    float padding = dataRange * 0.2f;
                                    float displayMinVal = dataMinVal - padding;
                                    float displayMaxVal = calcMaxVal + padding;
                                    float displayRange = displayMaxVal - displayMinVal;

                                    // 범위 안전성 검사
                                    if (!std::isfinite(displayMinVal) || !std::isfinite(displayMaxVal) || !std::isfinite(displayRange))
                                    {
                                        displayMinVal = 0.0f;
                                        displayMaxVal = 1.0f;
                                        displayRange = 1.0f;
                                    }

                                    // 키프레임 점 그리기 및 인터랙션
                                    ImGui::SetCursorScreenPos(canvas_pos);
                                    char canvas_id[32];
                                    sprintf_s(canvas_id, "canvas_vec_%d", axis);
                                    ImGui::InvisibleButton(canvas_id, canvas_size);
                                    bool is_hovered = ImGui::IsItemHovered();

                                    // Static 변수 대신 ImGui ID 사용 (축별로 독립)
                                    ImGuiStorage* storage = ImGui::GetStateStorage();
                                    char id_buf[64];
                                    sprintf_s(id_buf, "selected_vec_%d", axis);
                                    int state_key_selected = ImGui::GetID(id_buf);
                                    sprintf_s(id_buf, "dragging_vec_%d", axis);
                                    int state_key_dragging = ImGui::GetID(id_buf);
                                    sprintf_s(id_buf, "prev_mouse_x_vec_%d", axis);
                                    int state_key_prev_mouse_x_vec = ImGui::GetID(id_buf);
                                    sprintf_s(id_buf, "prev_mouse_y_vec_%d", axis);
                                    int state_key_prev_mouse_y_vec = ImGui::GetID(id_buf);
                                    sprintf_s(id_buf, "view_min_time_vec_%d", axis);
                                    int state_key_view_min_time = ImGui::GetID(id_buf);
                                    sprintf_s(id_buf, "view_max_time_vec_%d", axis);
                                    int state_key_view_max_time = ImGui::GetID(id_buf);
                                    sprintf_s(id_buf, "panning_vec_%d", axis);
                                    int state_key_panning = ImGui::GetID(id_buf);
                                    sprintf_s(id_buf, "pan_start_x_vec_%d", axis);
                                    int state_key_pan_start_x = ImGui::GetID(id_buf);

                                    int selected_point_vec = storage->GetInt(state_key_selected, -1);
                                    bool dragging_vec = storage->GetBool(state_key_dragging, false);

                                    // 줌/팬 상태 (기본값: MinTime~MaxTime 전체 범위)
                                    float view_min_time_vec = storage->GetFloat(state_key_view_min_time, Value->MinTime);
                                    float view_max_time_vec = storage->GetFloat(state_key_view_max_time, Value->MaxTime);
                                    bool panning_vec = storage->GetBool(state_key_panning, false);

                                    // Ctrl + 마우스 휠로 줌
                                    if (is_hovered && ImGui::GetIO().MouseWheel != 0.0f && ImGui::GetIO().KeyCtrl)
                                    {
                                        float wheel = ImGui::GetIO().MouseWheel;
                                        float mouse_t = (ImGui::GetMousePos().x - canvas_pos.x) / canvas_size.x;
                                        float zoom_t = view_min_time_vec + mouse_t * (view_max_time_vec - view_min_time_vec);
                                        float zoom_factor = wheel > 0 ? 0.9f : 1.1f;
                                        float new_range = (view_max_time_vec - view_min_time_vec) * zoom_factor;
                                        view_min_time_vec = zoom_t - (zoom_t - view_min_time_vec) * zoom_factor;
                                        view_max_time_vec = view_min_time_vec + new_range;

                                        // 범위 제한 (최소 0.01, MinTime~MaxTime 범위)
                                        float min_range = 0.01f;
                                        if (new_range < min_range) {
                                            view_min_time_vec = zoom_t - min_range * 0.5f;
                                            view_max_time_vec = zoom_t + min_range * 0.5f;
                                        }
                                        if (view_min_time_vec < Value->MinTime) view_min_time_vec = Value->MinTime;
                                        if (view_max_time_vec > Value->MaxTime) view_max_time_vec = Value->MaxTime;

                                        storage->SetFloat(state_key_view_min_time, view_min_time_vec);
                                        storage->SetFloat(state_key_view_max_time, view_max_time_vec);
                                    }

                                    // 중간 버튼으로 팬
                                    if (is_hovered && ImGui::IsMouseClicked(2))
                                    {
                                        panning_vec = true;
                                        storage->SetBool(state_key_panning, true);
                                        storage->SetFloat(state_key_pan_start_x, ImGui::GetMousePos().x);
                                    }
                                    if (panning_vec && ImGui::IsMouseDown(2))
                                    {
                                        float pan_start_x = storage->GetFloat(state_key_pan_start_x, ImGui::GetMousePos().x);
                                        float delta_x = ImGui::GetMousePos().x - pan_start_x;
                                        float delta_t = -(delta_x / canvas_size.x) * (view_max_time_vec - view_min_time_vec);
                                        view_min_time_vec += delta_t;
                                        view_max_time_vec += delta_t;
                                        storage->SetFloat(state_key_view_min_time, view_min_time_vec);
                                        storage->SetFloat(state_key_view_max_time, view_max_time_vec);
                                        storage->SetFloat(state_key_pan_start_x, ImGui::GetMousePos().x);
                                    }
                                    if (ImGui::IsMouseReleased(2))
                                    {
                                        panning_vec = false;
                                        storage->SetBool(state_key_panning, false);
                                    }

                                    float view_time_range_vec = view_max_time_vec - view_min_time_vec;

                                    // 배경
                                    draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(30, 30, 35, 255));
                                    draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(120, 120, 140, 255), 0.0f, 0, 1.5f);

                                    // 그리드
                                    for (int i = 0; i <= 10; ++i)
                                    {
                                        float alpha = (i == 0 || i == 10) ? 100.0f : 60.0f;
                                        float x = canvas_pos.x + (canvas_size.x * i / 10.0f);
                                        draw_list->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y), IM_COL32(70, 70, 80, (int)alpha));

                                        float y = canvas_pos.y + (canvas_size.y * i / 10.0f);
                                        draw_list->AddLine(ImVec2(canvas_pos.x, y), ImVec2(canvas_pos.x + canvas_size.x, y), IM_COL32(70, 70, 80, (int)alpha));
                                    }

                                    // 축 라벨 (시간 범위 표시)
                                    char time_label_vec[64];
                                    sprintf_s(time_label_vec, "Time (%.2f ~ %.2f)", view_min_time_vec, view_max_time_vec);
                                    draw_list->AddText(ImVec2(canvas_pos.x + canvas_size.x * 0.5f - 60.0f, canvas_pos.y + canvas_size.y + 5.0f), IM_COL32(200, 200, 200, 255), time_label_vec);
                                    draw_list->AddText(ImVec2(canvas_pos.x - AxisLabelWidth + 5.0f, canvas_pos.y + canvas_size.y * 0.5f), IM_COL32(200, 200, 200, 255), "Value");

                                    // 축 값 표시
                                    char buf_vec[32];
                                    sprintf_s(buf_vec, "%.2f", displayMaxVal);
                                    draw_list->AddText(ImVec2(canvas_pos.x - AxisLabelWidth + 5.0f, canvas_pos.y - 5.0f), IM_COL32(150, 150, 150, 255), buf_vec);
                                    sprintf_s(buf_vec, "%.2f", displayMinVal);
                                    draw_list->AddText(ImVec2(canvas_pos.x - AxisLabelWidth + 5.0f, canvas_pos.y + canvas_size.y - 10.0f), IM_COL32(150, 150, 150, 255), buf_vec);

                                    // 커브 선 그리기 (줌/팬 적용)
                                    if (Value->Curve.Points.Num() > 1)
                                    {
                                        for (int i = 0; i < Value->Curve.Points.Num() - 1; ++i)
                                        {
                                            const auto& p1 = Value->Curve.Points[i];
                                            const auto& p2 = Value->Curve.Points[i + 1];

                                            float v1 = (axis == 0) ? p1.OutVal.X : (axis == 1) ? p1.OutVal.Y : p1.OutVal.Z;
                                            float v2 = (axis == 0) ? p2.OutVal.X : (axis == 1) ? p2.OutVal.Y : p2.OutVal.Z;

                                            if (std::isfinite(v1) && std::isfinite(v2))
                                            {
                                                // 뷰 범위에 맞게 좌표 변환
                                                float norm_t1 = (p1.InVal - view_min_time_vec) / view_time_range_vec;
                                                float norm_t2 = (p2.InVal - view_min_time_vec) / view_time_range_vec;

                                                // 화면 밖 선분은 건너뛰기
                                                if ((norm_t1 < 0.0f && norm_t2 < 0.0f) || (norm_t1 > 1.0f && norm_t2 > 1.0f))
                                                    continue;

                                                float x1 = canvas_pos.x + FMath::Clamp(norm_t1, 0.0f, 1.0f) * canvas_size.x;
                                                float y1 = canvas_pos.y + canvas_size.y - FMath::Clamp((v1 - displayMinVal) / displayRange, 0.0f, 1.0f) * canvas_size.y;
                                                float x2 = canvas_pos.x + FMath::Clamp(norm_t2, 0.0f, 1.0f) * canvas_size.x;
                                                float y2 = canvas_pos.y + canvas_size.y - FMath::Clamp((v2 - displayMinVal) / displayRange, 0.0f, 1.0f) * canvas_size.y;

                                                draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), AxisColors[axis], 2.5f);
                                            }
                                        }
                                    }

                                    for (int i = 0; i < Value->Curve.Points.Num(); ++i)
                                    {
                                        auto& Point = Value->Curve.Points[i];
                                        float val = (axis == 0) ? Point.OutVal.X : (axis == 1) ? Point.OutVal.Y : Point.OutVal.Z;
                                        if (!std::isfinite(val)) continue;

                                        // 뷰 범위에 맞게 좌표 변환
                                        float norm_t = (Point.InVal - view_min_time_vec) / view_time_range_vec;
                                        float x = canvas_pos.x + FMath::Clamp(norm_t, 0.0f, 1.0f) * canvas_size.x;
                                        float y = canvas_pos.y + canvas_size.y - FMath::Clamp((val - displayMinVal) / displayRange, 0.0f, 1.0f) * canvas_size.y;

                                        bool is_point_hovered = ImGui::IsMouseHoveringRect(ImVec2(x - 6, y - 6), ImVec2(x + 6, y + 6));

                                        if (is_point_hovered && ImGui::IsMouseClicked(0))
                                        {
                                            selected_point_vec = i;
                                            dragging_vec = true;
                                            storage->SetInt(state_key_selected, i);
                                            storage->SetBool(state_key_dragging, true);
                                            // 드래그 시작 시 마우스 위치 저장
                                            ImVec2 mouse_pos = ImGui::GetMousePos();
                                            storage->SetFloat(state_key_prev_mouse_x_vec, mouse_pos.x);
                                            storage->SetFloat(state_key_prev_mouse_y_vec, mouse_pos.y);
                                        }

                                        if (dragging_vec && selected_point_vec == i && ImGui::IsMouseDown(0))
                                        {
                                            ImVec2 mouse_pos = ImGui::GetMousePos();
                                            float prev_mouse_x = storage->GetFloat(state_key_prev_mouse_x_vec, mouse_pos.x);
                                            float prev_mouse_y = storage->GetFloat(state_key_prev_mouse_y_vec, mouse_pos.y);

                                            // 마우스 이동량 계산
                                            float delta_x = mouse_pos.x - prev_mouse_x;
                                            float delta_y = mouse_pos.y - prev_mouse_y;

                                            // 감도 설정 (1.0 = 정상 속도)
                                            const float sensitivity = 1.0f;

                                            // X축: Time (뷰 범위 적용)
                                            float time_delta = (delta_x / canvas_size.x) * view_time_range_vec * sensitivity;
                                            Point.InVal += time_delta;

                                            // 시간 값을 MinTime~MaxTime 범위로 제한
                                            Point.InVal = FMath::Clamp(Point.InVal, Value->MinTime, Value->MaxTime);

                                            // Y축: Value
                                            float value_delta = -(delta_y / canvas_size.y) * displayRange * sensitivity;
                                            float newVal = val + value_delta;
                                            newVal = FMath::Clamp(newVal, -100000.0f, 100000.0f);

                                            // NaN/Inf 방지
                                            if (std::isfinite(newVal))
                                            {
                                                if (axis == 0) Point.OutVal.X = newVal;
                                                else if (axis == 1) Point.OutVal.Y = newVal;
                                                else Point.OutVal.Z = newVal;
                                            }

                                            // 현재 마우스 위치 저장
                                            storage->SetFloat(state_key_prev_mouse_x_vec, mouse_pos.x);
                                            storage->SetFloat(state_key_prev_mouse_y_vec, mouse_pos.y);

                                            bChanged = true;
                                        }

                                        if (is_point_hovered && ImGui::IsMouseClicked(1))
                                        {
                                            Value->Curve.Points.RemoveAt(i);
                                            storage->SetInt(state_key_selected, -1);
                                            bChanged = true;
                                            break;
                                        }

                                        ImU32 color = (selected_point_vec == i) ? IM_COL32(255, 255, 255, 255) : AxisColors[axis];
                                        draw_list->AddCircleFilled(ImVec2(x, y), 6.0f, color);
                                        draw_list->AddCircle(ImVec2(x, y), 6.0f, IM_COL32(0, 0, 0, 255), 16, 2.0f);

                                        // 호버 시 값 표시
                                        if (is_point_hovered)
                                        {
                                            char tooltip[64];
                                            sprintf_s(tooltip, "T: %.3f\nV: %.3f", Point.InVal, val);
                                            ImGui::SetTooltip("%s", tooltip);
                                        }
                                    }

                                    if (ImGui::IsMouseReleased(0))
                                    {
                                        storage->SetBool(state_key_dragging, false);
                                        if (selected_point_vec != -1)
                                        {
                                            std::sort(Value->Curve.Points.begin(), Value->Curve.Points.end(),
                                                [](const FInterpCurvePoint<FVector>& A, const FInterpCurvePoint<FVector>& B)
                                                {
                                                    return A.InVal < B.InVal;
                                                });
                                        }
                                    }

                                    if (is_hovered && ImGui::IsMouseDoubleClicked(0))
                                    {
                                        ImVec2 mouse_pos = ImGui::GetMousePos();
                                        float norm_x = (mouse_pos.x - canvas_pos.x) / canvas_size.x;
                                        float t = view_min_time_vec + norm_x * view_time_range_vec;

                                        // 시간 값을 MinTime~MaxTime 범위로 제한
                                        t = FMath::Clamp(t, Value->MinTime, Value->MaxTime);

                                        float normalized_y = 1.0f - (mouse_pos.y - canvas_pos.y) / canvas_size.y;
                                        // normalized_y를 합리적인 범위로 제한
                                        normalized_y = FMath::Clamp(normalized_y, -1.0f, 2.0f);
                                        float v = displayMinVal + normalized_y * displayRange;
                                        v = FMath::Clamp(v, -100000.0f, 100000.0f);

                                        if (std::isfinite(v))
                                        {
                                            // 해당 시간에서의 기존 값을 가져오거나 보간
                                            FVector newVec = Value->Curve.HasKeys() ? Value->Curve.Eval(t) : FVector::Zero();

                                            // 현재 축만 업데이트
                                            if (axis == 0) newVec.X = v;
                                            else if (axis == 1) newVec.Y = v;
                                            else newVec.Z = v;

                                            Value->Curve.AddPoint(t, newVec);
                                            bChanged = true;
                                        }
                                    }

                                    // 하단 여백 확보
                                    ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x, canvas_pos.y + canvas_size.y + AxisLabelHeight + 5.0f));
                                    ImGui::Dummy(ImVec2(0, 5));

                                    ImGui::TreePop();
                                }
                                ImGui::PopID();
                            }

                            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "LClick: Select | Drag: Move | RClick: Delete | DblClick: Add | Ctrl+MWheel: Zoom | MButton: Pan");
                        }
                        ImGui::PopID();
                    }
                }
            }

            if (!bHasCurveProperty)
            {
                ImGui::TextWrapped("No Curve properties found.");
                ImGui::Spacing();
                ImGui::TextWrapped("Set a Distribution property's Mode to 'Curve' in the Detail panel to edit curves here.");
            }
        }
        else
        {
            ImGui::Text("No module or emitter selected.");
        }

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
        ImGui::SameLine();
        if (ImGui::Button("이미터 삭제"))
        {
            if (State->GetCachedParticle()->RemoveEmitter(State->SelectedEmitter))
            {
                State->SelectedEmitter = nullptr;
                State->SelectedLODLevel = 0;
                State->SelectedModule = nullptr;
                State->ReStartParticle();
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
