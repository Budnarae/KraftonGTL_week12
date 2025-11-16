#include "pch.h"
#include "SViewerWindowBase.h"
#include "ImGui/imgui.h"

SViewerWindowBase::SViewerWindowBase()
    : World(nullptr)
    , Device(nullptr)
    , bIsOpen(true)
    , bInitialPlacementDone(false)
    , bRequestFocus(false)
    , ActiveState(nullptr)
    , ActiveTabIndex(-1)
{
}

SViewerWindowBase::~SViewerWindowBase()
{
    // 자식 클래스가 소멸자에서 탭을 정리해야 합니다
    // 여기서는 배열만 비웁니다
    Tabs.Empty();
    ActiveState = nullptr;
}

bool SViewerWindowBase::Initialize(ID3D11Device* InDevice, UWorld* InWorld)
{
    Device = InDevice;
    World = InWorld;
    return true;
}

void SViewerWindowBase::OnRender()
{
    // 기본 구현: 자식 클래스에서 오버라이드
}

void SViewerWindowBase::OnUpdate(float DeltaSeconds)
{
    // 기본 구현: 자식 클래스에서 오버라이드
}

void SViewerWindowBase::RenderTabBar()
{
    if (ImGui::BeginTabBar("ViewerTabs", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable))
    {
        for (int i = 0; i < Tabs.Num(); ++i)
        {
            ViewerTabStateBase* State = Tabs[i];
            bool open = true;
            if (ImGui::BeginTabItem(State->Name.c_str(), &open))
            {
                ActiveTabIndex = i;
                ActiveState = State;
                ImGui::EndTabItem();
            }
            if (!open)
            {
                CloseTab(i);
                break;
            }
        }

        // + 버튼으로 새 탭 추가
        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
        {
            char label[32];
            sprintf_s(label, "Tab %d", Tabs.Num() + 1);
            OpenNewTab(label);
        }

        ImGui::EndTabBar();
    }
}

void SViewerWindowBase::OpenNewTab(const char* Name)
{
    ViewerTabStateBase* NewState = CreateTabState(Name);
    if (!NewState) return;

    Tabs.Add(NewState);
    ActiveTabIndex = Tabs.Num() - 1;
    ActiveState = NewState;
}

void SViewerWindowBase::CloseTab(int Index)
{
    if (Index < 0 || Index >= Tabs.Num()) return;

    ViewerTabStateBase* State = Tabs[Index];
    DestroyTabState(State);
    Tabs.RemoveAt(Index);

    if (Tabs.Num() == 0)
    {
        ActiveTabIndex = -1;
        ActiveState = nullptr;
    }
    else
    {
        ActiveTabIndex = std::min(Index, Tabs.Num() - 1);
        ActiveState = Tabs[ActiveTabIndex];
    }
}
