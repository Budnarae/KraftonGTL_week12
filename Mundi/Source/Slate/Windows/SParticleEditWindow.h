#pragma once
#include "SViewerWindowBase.h"
#include "Source/Slate/Widgets/AssetBrowserWidget.h"
#include "Source/Runtime/Engine/ParticleEditor/ParticleViewerState.h"

class UParticleModuleRequired;
class FViewport;
class FViewportClient;
class SParticleEditWindow;
struct ID3D11Device;

enum class EParticleEditorWindow
{
    None,
    Viewport,
    Emitter,
    Detail
};
enum class EMenuActionType
{
    AddEmitter,
    AddSpawnModule,
    AddUpdateModule,
};
struct FMenuAction
{
    EMenuActionType MenuActionType;
    int EmitterOffset;
    FString ClassName;

    static FMenuAction CreateSpawnModule(const FString& InClassName)
    {
        FMenuAction Action;
        Action.MenuActionType = EMenuActionType::AddSpawnModule;
        Action.ClassName = InClassName;
        return Action;
    }
    static FMenuAction CreateUpdateModule(const FString& InClassName)
    {
        FMenuAction Action;
        Action.MenuActionType = EMenuActionType::AddUpdateModule;
        Action.ClassName = InClassName;
        return Action;
    }
    static FMenuAction CreateAddEmitter(const int InEmitterOffset)
    {
        FMenuAction Action;
        Action.MenuActionType = EMenuActionType::AddEmitter;
        Action.EmitterOffset = InEmitterOffset;
        return Action;
    }
    void Action(SParticleEditWindow* ParticleEditWindow) const;
private:
    FMenuAction() = default;
};

class SParticleEditWindow : public SViewerWindowBase
{
public:
    static void CreateParticleEditor(const FString& Path);
    static void CreateParticleEditor(const UParticleModuleRequired* ParticleModule);

public:
    SParticleEditWindow();
    virtual ~SParticleEditWindow();

    // 베이스 클래스의 Initialize 오버라이드 (통합 API용)
    virtual bool Initialize(ID3D11Device* InDevice, UWorld* InWorld) override;

    // 커스텀 Initialize (위치/크기 지정)
    bool Initialize(float StartX, float StartY, float Width, float Height, UWorld* InWorld, ID3D11Device* InDevice);

    // SViewerWindowBase overrides
    virtual void LoadAsset(const FString& AssetPath) override;

    // SWindow overrides
    virtual void OnRender() override;
    virtual void OnUpdate(float DeltaSeconds) override;
    virtual void OnMouseMove(FVector2D MousePos) override;
    virtual void OnMouseDown(FVector2D MousePos, uint32 Button) override;
    virtual void OnMouseUp(FVector2D MousePos, uint32 Button) override;

    void OnRenderViewport();

    UParticleSystem* GetParticleSystem()
    {
        return &State->PreviewParticle->ParticleSystem;
    }
    void AddEmitter(const int EmitterOffset);
    void AddSpawnModule(const FString& ClassName);
    void AddUpdateModule(const FString& ClassName);


private:

    ParticleViewerState* State = nullptr;

    // Layout state
    float LeftPanelRatio = 0.25f;   // 25% of width
    float RightPanelRatio = 0.25f;  // 25% of width


    EParticleEditorWindow HoveredWindow;
    ImVec2 EmitterDropdownPos;
    bool bEmitterDropdown = false;

    // Cached center region used for viewport sizing and input mapping
    FRect CenterRect;

    // UI Widgets (재사용 가능)
    FAssetBrowserWidget AssetBrowser;
    

private:
    void Save();
    void ReStart();

    void DrawEmitterView();
    void DrawModuleInEmitterView(UParticleModule* Module, const ImVec2& Size);
    void DrawEmitterDropdown();

};
