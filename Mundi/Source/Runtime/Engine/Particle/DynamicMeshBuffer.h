#pragma once

#include "VertexData.h"

struct ID3D11Buffer;

// Beam/Ribbon 렌더링용 동적 버퍼 관리 클래스
// 매 프레임 정점/인덱스가 변경되는 메시를 위한 D3D11_USAGE_DYNAMIC 버퍼를 관리합니다.
class FDynamicMeshBuffer
{
public:
    FDynamicMeshBuffer() = default;
    ~FDynamicMeshBuffer();

    // 복사 방지
    FDynamicMeshBuffer(const FDynamicMeshBuffer&) = delete;
    FDynamicMeshBuffer& operator=(const FDynamicMeshBuffer&) = delete;

    // 이동 허용
    FDynamicMeshBuffer(FDynamicMeshBuffer&& Other) noexcept;
    FDynamicMeshBuffer& operator=(FDynamicMeshBuffer&& Other) noexcept;

    // 버퍼 업데이트 (필요시 재할당)
    // @return 성공 여부
    bool UpdateVertices(const TArray<FBillboardVertex>& Vertices);
    bool UpdateIndices(const TArray<uint32>& Indices);

    // 한번에 정점과 인덱스 모두 업데이트
    bool Update(const TArray<FBillboardVertex>& Vertices, const TArray<uint32>& Indices);

    // 리소스 해제
    void Release();

    // Getters
    ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
    ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer; }
    uint32 GetVertexCount() const { return VertexCount; }
    uint32 GetIndexCount() const { return IndexCount; }
    uint32 GetVertexStride() const { return sizeof(FBillboardVertex); }
    bool IsValid() const { return VertexBuffer != nullptr && IndexBuffer != nullptr; }

private:
    // 버퍼 용량이 부족하면 재할당
    bool EnsureVertexCapacity(uint32 RequiredCount);
    bool EnsureIndexCapacity(uint32 RequiredCount);

    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;

    uint32 VertexCapacity = 0;  // 현재 버퍼가 수용 가능한 최대 정점 수
    uint32 IndexCapacity = 0;   // 현재 버퍼가 수용 가능한 최대 인덱스 수

    uint32 VertexCount = 0;     // 실제 사용 중인 정점 수
    uint32 IndexCount = 0;      // 실제 사용 중인 인덱스 수
};
