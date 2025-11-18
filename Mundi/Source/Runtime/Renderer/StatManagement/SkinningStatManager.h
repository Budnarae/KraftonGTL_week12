#pragma once

class FSkinningStatManager
{
public:
    static FSkinningStatManager& GetInstance();

    void BeginFrame();

    void RecordStart();
    void RecordEnd();

    void RecordCPUStart();
    void RecordCPUEnd();

    double GetFinalRecordTime();
    double GetGPURecordTime();
    double GetCPURecordTime();
private:
    FSkinningStatManager();
    ~FSkinningStatManager();

    double GetRecordTime();

    void Initialize();
    void Shutdown();
private:
    ID3D11Device* pDevice{};
    ID3D11DeviceContext* pContext{};

    // GPU 쿼리 링 버퍼 (4개 셋)
    static constexpr int NUM_QUERY_SETS = 4;
    ID3D11Query* g_QueryStart[NUM_QUERY_SETS] = {};
    ID3D11Query* g_QueryEnd[NUM_QUERY_SETS] = {};
    ID3D11Query* g_QueryDisjoint[NUM_QUERY_SETS] = {};

    int g_WriteIndex = 0;  // 현재 프레임에서 쓸 쿼리 인덱스
    int g_ReadIndex = 0;   // 읽을 쿼리 인덱스 (WriteIndex - 3)

    std::chrono::high_resolution_clock::time_point CpuStartTime{};
    std::chrono::high_resolution_clock::time_point CpuEndTime{};

    // 프레임당 누적 시간
    double AccumulatedCPUTime = 0.0;
    double CachedGPUTime = 0.0;
};