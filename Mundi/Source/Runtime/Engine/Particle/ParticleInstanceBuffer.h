#pragma once

#include "ParticleData.h"

struct ID3D11Buffer;
struct ID3D11ShaderResourceView;

// 파티클 인스턴스 버퍼 관리자
// 모든 파티클 컴포넌트가 공유하는 단일 StructuredBuffer를 관리합니다.
class FParticleInstanceBufferManager
{
public:
    static FParticleInstanceBufferManager& Get();

    // 버퍼 초기화 (엔진 시작 시 호출)
    void Initialize(int32 MaxParticleCount = 10000);

    // 버퍼 해제 (엔진 종료 시 호출)
    void Release();

    // 인스턴스 데이터 업데이트 및 SRV 반환
    ID3D11ShaderResourceView* UpdateAndGetSRV(const TArray<FParticleInstanceData>& InstanceData);

    // 최대 파티클 수 반환
    int32 GetMaxParticleCount() const { return MaxParticleCount; }

private:
    FParticleInstanceBufferManager() = default;
    ~FParticleInstanceBufferManager();

    // 복사 방지
    FParticleInstanceBufferManager(const FParticleInstanceBufferManager&) = delete;
    FParticleInstanceBufferManager& operator=(const FParticleInstanceBufferManager&) = delete;

    ID3D11Buffer* InstanceBuffer = nullptr;
    ID3D11ShaderResourceView* InstanceSRV = nullptr;
    int32 MaxParticleCount = 10;
};
