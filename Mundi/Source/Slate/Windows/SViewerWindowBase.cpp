#include "pch.h"
#include "SViewerWindowBase.h"
#include "ImGui/imgui.h"

SViewerWindowBase::SViewerWindowBase()
    : World(nullptr)
    , Device(nullptr)
    , bIsOpen(true)
    , bInitialPlacementDone(false)
    , bRequestFocus(false)
{
}

SViewerWindowBase::~SViewerWindowBase()
{
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
