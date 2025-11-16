#include "pch.h"
#include "VertexData.h"
#include "Source/Runtime/Engine/Animation/AnimNodeTransitionRule.h"

void FBillboardVertex::FillFrom(const FMeshData& mesh, size_t i)
{
    WorldPosition = mesh.Vertices[i];
    UV = (i < mesh.UV.size()) ? mesh.UV[i] : FVector2D(0.0f, 0.0f);
}

void FBillboardVertex::FillFrom(const FNormalVertex& src)
{
    WorldPosition = src.pos;
    UV = src.tex;
}

void FVertexSimple::FillFrom(const FMeshData& mesh, size_t i)
{
    Position = mesh.Vertices[i];
    Color = (i < mesh.Color.size()) ? mesh.Color[i] : FVector4(1, 1, 1, 1);
}

void FVertexSimple::FillFrom(const FNormalVertex& src)
{
    Position = src.pos;
    Color = src.color;
}

void FVertexDynamic::FillFrom(const FMeshData& mesh, size_t i)
{
    Position = mesh.Vertices[i];
    Color = (i < mesh.Color.size()) ? mesh.Color[i] : FVector4(1, 1, 1, 1);
    UV = (i < mesh.UV.size()) ? mesh.UV[i] : FVector2D(0, 0);
    Normal = (i < mesh.Normal.size()) ? mesh.Normal[i] : FVector(0, 0, 1);
}

void FVertexDynamic::FillFrom(const FNormalVertex& src)
{
    Position = src.pos;
    Color = src.color;
    UV = src.tex;
    Tangent = src.Tangent;
    Normal = FVector{ src.normal.X, src.normal.Y, src.normal.Z };
}

void FBillboardVertexInfo_GPU::FillFrom(const FMeshData& mesh, size_t i)
{
    Position[0] = mesh.Vertices[i].X;
    Position[1] = mesh.Vertices[i].Y;
    Position[2] = mesh.Vertices[i].Z;
    CharSize[0] = (i < mesh.UV.size()) ? mesh.UV[i].X : 1.0f;
    CharSize[1] = (i < mesh.UV.size()) ? mesh.UV[i].Y : 1.0f;
    if (i < mesh.Color.size()) {
        UVRect[0] = mesh.Color[i].X;
        UVRect[1] = mesh.Color[i].Y;
        UVRect[2] = mesh.Color[i].Z;
        UVRect[3] = mesh.Color[i].W;
    }
    else {
        UVRect[0] = 0; UVRect[1] = 0; UVRect[2] = 1; UVRect[3] = 1;
    }
}

void FBillboardVertexInfo_GPU::FillFrom(const FNormalVertex& src)
{
    Position[0] = src.pos.X;
    Position[1] = src.pos.Y;
    Position[2] = src.pos.Z;

    CharSize[0] = src.tex.X;
    CharSize[1] = src.tex.Y;

    UVRect[0] = src.color.X;
    UVRect[1] = src.color.Y;
    UVRect[2] = src.color.Z;
    UVRect[3] = src.color.W;
}

// ====================================
// FAnimStateTransition 구현
// ====================================

void FAnimStateTransition::CleanupDelegate()
{
    if (AssociatedRule && DelegateHandle != 0)
    {
        AssociatedRule->GetTransitionDelegate().Remove(DelegateHandle);
        DelegateHandle = 0;
        AssociatedRule = nullptr;
    }
}

FAnimStateTransition::~FAnimStateTransition()
{
    CleanupDelegate();
}

FAnimStateTransition::FAnimStateTransition(const FAnimStateTransition& Other)
    : SourceState(Other.SourceState)
    , TargetState(Other.TargetState)
    , Index(Other.Index)
    , CanEnterTransition(false)  // 새로 생성되는 Transition은 항상 false로 초기화
    , AssociatedRule(nullptr)  // Delegate는 복사하지 않음
    , DelegateHandle(0)
    , BlendTime(Other.BlendTime)
    , BlendTimeElapsed(Other.BlendTimeElapsed)
    , BlendAlpha(Other.BlendAlpha)
    , bIsBlending(Other.bIsBlending)
{
    // Delegate는 복사 후 재바인딩 필요
}

FAnimStateTransition& FAnimStateTransition::operator=(const FAnimStateTransition& Other)
{
    if (this != &Other)
    {
        // 기존 Delegate 정리
        CleanupDelegate();

        SourceState = Other.SourceState;
        TargetState = Other.TargetState;
        Index = Other.Index;
        CanEnterTransition = Other.CanEnterTransition;
        BlendTime = Other.BlendTime;
        BlendTimeElapsed = Other.BlendTimeElapsed;
        BlendAlpha = Other.BlendAlpha;
        bIsBlending = Other.bIsBlending;

        // Delegate는 복사하지 않음 (재바인딩 필요)
        AssociatedRule = nullptr;
        DelegateHandle = 0;
    }
    return *this;
}
