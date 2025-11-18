#include "pch.h"
#include "SkinningStatManager.h"
#include "Source/Runtime/Engine/Components/SkinnedMeshComponent.h"

FSkinningStatManager& FSkinningStatManager::GetInstance()
{
    static FSkinningStatManager instance;
    return instance;
}

FSkinningStatManager::FSkinningStatManager()
{
    Initialize();
}

FSkinningStatManager::~FSkinningStatManager()
{
   Shutdown();
}

void FSkinningStatManager::Initialize()
{
    // 중복 호출 방어: 이미 초기화되어 있으면 먼저 해제
    Shutdown();

    if (!GEngine.GetRHIDevice())
        assert(false);

    pDevice = GEngine.GetRHIDevice()->GetDevice();
    pContext = GEngine.GetRHIDevice()->GetDeviceContext();

    if (!pDevice || !pContext)
        assert(false);

    // Timestamp 쿼리 설정
    D3D11_QUERY_DESC descTimestamp = {};
    descTimestamp.Query = D3D11_QUERY_TIMESTAMP;

    // Disjoint 쿼리 설정 (주파수 및 유효성 확인용)
    D3D11_QUERY_DESC descDisjoint = {};
    descDisjoint.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

    // 링 버퍼용 쿼리 셋 생성 (4개)
    for (int i = 0; i < NUM_QUERY_SETS; ++i)
    {
        pDevice->CreateQuery(&descTimestamp, &g_QueryStart[i]);
        pDevice->CreateQuery(&descTimestamp, &g_QueryEnd[i]);
        pDevice->CreateQuery(&descDisjoint, &g_QueryDisjoint[i]);
    }
}

void FSkinningStatManager::Shutdown()
{
    // 링 버퍼 모든 쿼리 해제
    for (int i = 0; i < NUM_QUERY_SETS; ++i)
    {
        if (g_QueryStart[i])
        {
            g_QueryStart[i]->Release();
            g_QueryStart[i] = nullptr;
        }

        if (g_QueryEnd[i])
        {
            g_QueryEnd[i]->Release();
            g_QueryEnd[i] = nullptr;
        }

        if (g_QueryDisjoint[i])
        {
            g_QueryDisjoint[i]->Release();
            g_QueryDisjoint[i] = nullptr;
        }
    }
}

void FSkinningStatManager::RecordStart()
{
    // 현재 프레임용 쿼리 사용
    pContext->Begin(g_QueryDisjoint[g_WriteIndex]);
    pContext->End(g_QueryStart[g_WriteIndex]);
}

void FSkinningStatManager::RecordEnd()
{
    // 현재 프레임용 쿼리 종료
    pContext->End(g_QueryEnd[g_WriteIndex]);
    pContext->End(g_QueryDisjoint[g_WriteIndex]);

    // 다음 프레임을 위해 Write Index 증가 (링 버퍼)
    g_WriteIndex = (g_WriteIndex + 1) % NUM_QUERY_SETS;

    // Read Index 업데이트 (Write보다 3프레임 뒤)
    g_ReadIndex = (g_WriteIndex + 1) % NUM_QUERY_SETS;
}

double FSkinningStatManager::GetRecordTime()
{
    // ReadIndex의 쿼리 결과 읽기 (3프레임 전 쿼리)
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData = {};
    UINT64 timestampStart = 0;
    UINT64 timestampEnd = 0;

    // 1. Disjoint 쿼리 결과 읽기 (GPU가 결과를 준비했는지 확인)
    HRESULT hr = pContext->GetData
    (
        g_QueryDisjoint[g_ReadIndex],
        &disjointData,
        sizeof(disjointData),
        0
    );

    // S_FALSE이면 데이터가 준비되지 않았으므로 캐시된 값 반환
    if (hr != S_OK)
    {
        return CachedGPUTime;
    }

    // 2. 클럭 유효성 확인 (클럭 주파수가 안정적인지 확인)
    if (disjointData.Disjoint == TRUE)
    {
        return CachedGPUTime;
    }

    // 3. Start, End 타임스탬프 값 읽기
    pContext->GetData(g_QueryStart[g_ReadIndex], &timestampStart, sizeof(timestampStart), 0);
    pContext->GetData(g_QueryEnd[g_ReadIndex], &timestampEnd, sizeof(timestampEnd), 0);

    // 4. 시간 계산 (단위: 밀리초)
    UINT64 deltaTicks = timestampEnd - timestampStart;
    double timeSeconds = (double)deltaTicks / (double)disjointData.Frequency;
    double timeMilliseconds = timeSeconds * 1000.0;

    return timeMilliseconds;
}

void FSkinningStatManager::RecordCPUStart()
{
    CpuStartTime = std::chrono::high_resolution_clock::now();
}

void FSkinningStatManager::RecordCPUEnd()
{
    CpuEndTime = std::chrono::high_resolution_clock::now();

    // 이번 컴포넌트의 CPU 시간을 누적
    std::chrono::duration<double, std::milli> CpuTimeMs = CpuEndTime - CpuStartTime;
    AccumulatedCPUTime += CpuTimeMs.count();
}

void FSkinningStatManager::BeginFrame()
{
    // 3프레임 전 GPU 쿼리 결과 읽기
    CachedGPUTime = GetRecordTime();

    // CPU 시간 초기화
    AccumulatedCPUTime = 0.0;
}

double FSkinningStatManager::GetCPURecordTime()
{
    return AccumulatedCPUTime;
}

double FSkinningStatManager::GetGPURecordTime()
{
    return CachedGPUTime;
}

double FSkinningStatManager::GetFinalRecordTime()
{
    // GPU 스키닝: GPU 시간만 (셰이더에서 스키닝)
    // CPU 스키닝: CPU 시간 + GPU 파이프라인 시간
    if (USkinnedMeshComponent::IsGlobalGpuSkinningEnabled())
        return GetGPURecordTime();
    else
        return GetCPURecordTime() + GetGPURecordTime();
}