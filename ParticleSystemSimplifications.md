# 파티클 시스템 단순화 사항

이 문서는 Mundi 엔진의 파티클 시스템 구현에서 언리얼 엔진 대비 단순화하거나 생략한 사항들을 기록합니다.

---

## 1. 멀티스레드 관련 단순화

### 언리얼 엔진
- Game Thread와 Render Thread가 분리되어 병렬 실행
- `GetDynamicData()`로 시뮬레이션 데이터의 스냅샷을 생성하여 Render Thread에 전달
- `FParticleDataContainer`가 별도의 메모리 블록을 할당하여 데이터 복사
- 스레드 간 동기화 및 더블 버퍼링 필요

### Mundi 엔진 (단순화)
- **싱글 스레드 실행**: Game Thread와 Render Thread가 분리되지 않음
- **직접 참조**: `FDynamicSpriteEmitterData`가 `FParticleEmitterInstance`의 `ParticleData`를 직접 참조
- **메모리 복사 생략**: 시뮬레이션 데이터를 복사하지 않고 포인터만 저장
- **인덱스 배열만 별도 할당**: 정렬을 위한 `ParticleIndices`만 새로 할당

---

## 2. 생략된 기능들

### 2.1 Dynamic Buffer / GPU 버퍼 관리
- **언리얼**: `FParticleDynamicData`가 GPU 업로드를 위한 동적 버퍼(Vertex Buffer) 생성
- **Mundi**: 동적 버퍼 생성 생략, 각 파티클마다 개별 Draw Call 수행
  - `GetDynamicMeshElementsEmitter()` 미구현
  - 인스턴싱(Instancing) 미구현

### 2.2 직렬화 (Serialization)
- `FDynamicEmitterReplayDataBase::Serialize()` 미구현
- 파티클 리플레이/녹화 기능 없음

### 2.3 LOD (Level of Detail) 시스템
- LOD 레벨 변경 로직 단순화 (항상 LOD 0 사용)
- `SetCurrentLODLevel()`은 구현되어 있으나 실제 LOD 전환 없음

### 2.4 다중 에미터 타입
- **구현됨**: `EDET_Sprite` (스프라이트 에미터)
- **미구현**:
  - `EDET_Mesh` (메시 파티클)
  - `EDET_Beam` (빔 파티클)
  - `EDET_Ribbon` (리본 파티클)

### 2.5 고급 정렬 모드
- **구현됨**: Back-to-Front 거리 정렬 (버블 정렬)
- **미구현**:
  - View Z 정렬
  - Age 기반 정렬
  - 사용자 정의 정렬

---

## 3. 렌더링 파이프라인 단순화

### 언리얼 엔진 흐름
```
EmitterInstance::Tick()
    ↓
GetDynamicData() - 스냅샷 생성 및 메모리 복사
    ↓
FDynamicSpriteEmitterData::Init() - Render Thread용 데이터 초기화
    ↓
GetDynamicMeshElementsEmitter() - GPU 버퍼 생성 및 업로드
    ↓
FMeshElementCollector - 메시 배치 수집
    ↓
렌더링
```

### Mundi 엔진 흐름 (단순화)
```
EmitterInstance::Update() / SpawnParticles()
    ↓
CreateDynamicData() - 포인터 참조 설정, 인덱스 배열 할당
    ↓
CollectMeshBatches() - 카메라 기준 정렬, FMeshBatchElement 생성
    ↓
렌더링
```

---

## 4. 메모리 관리 차이

### FParticleDataContainer
- **언리얼**: `ParticleData`와 `ParticleIndices`를 단일 메모리 블록에 연속 배치
  ```cpp
  void AllocateMemory(int32 MaxParticleCount, int32 ParticleStride)
  {
      // ParticleData 뒤에 ParticleIndices가 이어짐
      MemBlockSize = (MaxParticleCount * ParticleStride) + (MaxParticleCount * sizeof(uint16));
      ParticleData = malloc(MemBlockSize);
      ParticleIndices = (uint16*)(ParticleData + MaxParticleCount * ParticleStride);
  }
  ```

- **Mundi**: 시뮬레이션 데이터 참조, 인덱스만 별도 할당
  ```cpp
  void FDynamicSpriteEmitterData::Init(FParticleEmitterInstance* Instance, int32 Index)
  {
      // 시뮬레이션 데이터는 직접 참조
      Source.DataContainer.ParticleData = Instance->ParticleData;

      // 인덱스 배열만 새로 할당
      Source.DataContainer.ParticleIndices = malloc(Instance->ActiveParticles * sizeof(uint16));
  }
  ```

---

## 5. 성능 고려사항

### 현재 구현의 한계
1. **Draw Call 과다**: 파티클 수만큼 Draw Call 발생
2. **버블 정렬**: O(n²) 복잡도, 파티클 수가 많으면 성능 저하
3. **매 프레임 메모리 할당**: `CreateDynamicData()`에서 인덱스 배열 재할당

### 추후 최적화 방향
1. **GPU 인스턴싱**: 단일 Draw Call로 모든 파티클 렌더링
2. **정렬 알고리즘 개선**: Radix Sort 또는 GPU 기반 정렬
3. **메모리 풀링**: 인덱스 배열 재사용

---

## 6. 파일 변경 내역

### 신규 추가
- `ParticleData.h`: `FDynamicSpriteEmitterData` 구조체
- `ParticleData.cpp`: `Init()`, `SortParticles()`, `Release()` 구현

### 수정
- `ParticleSystemComponent.h`:
  - `TArray<FDynamicSpriteEmitterData*> DynamicEmitterData` 멤버 추가
  - `CreateDynamicData()`, `ReleaseDynamicData()` 함수 추가

- `ParticleSystemComponent.cpp`:
  - `TickComponent()`: 시뮬레이션 후 `CreateDynamicData()` 호출
  - `Deactivate()`: `ReleaseDynamicData()` 호출
  - `CollectMeshBatches()`: `DynamicEmitterData` 사용, 정렬된 인덱스로 파티클 접근

---

## 7. GPU 인스턴싱 구현 가이드

### 7.1 셰이더 구조 (완료)

**UberLit.hlsl**에 `PARTICLE_SPRITE` 매크로 지원이 추가되었습니다:

```hlsl
// 파티클 인스턴스 데이터 구조체
struct FParticleInstanceData
{
    float3 Position;    // 월드 위치
    float Rotation;     // 회전 각도 (라디안)
    float2 Size;        // 파티클 크기
    float2 Padding;     // 16바이트 정렬
    float4 Color;       // 파티클 색상
};

StructuredBuffer<FParticleInstanceData> g_ParticleInstances : register(t12);
```

### 7.2 C++ 측 구현 필요 사항

**ParticleSystemComponent**에 다음을 추가해야 합니다:

```cpp
// 멤버 변수
ID3D11Buffer* ParticleInstanceBuffer = nullptr;
ID3D11ShaderResourceView* ParticleInstanceSRV = nullptr;
int32 MaxInstanceCount = 0;

// 버퍼 생성 (Activate에서 호출)
void CreateInstanceBuffer(int32 MaxParticles)
{
    D3D11RHI* RHI = GetRHI();
    RHI->CreateStructuredBuffer(
        sizeof(FParticleInstanceData),
        MaxParticles,
        nullptr,
        &ParticleInstanceBuffer
    );
    RHI->CreateStructuredBufferSRV(ParticleInstanceBuffer, &ParticleInstanceSRV);
    MaxInstanceCount = MaxParticles;
}

// 버퍼 해제 (Deactivate에서 호출)
void ReleaseInstanceBuffer()
{
    if (ParticleInstanceSRV) { ParticleInstanceSRV->Release(); ParticleInstanceSRV = nullptr; }
    if (ParticleInstanceBuffer) { ParticleInstanceBuffer->Release(); ParticleInstanceBuffer = nullptr; }
}
```

### 7.3 CollectMeshBatches 수정

인스턴싱 렌더링을 위해 `CollectMeshBatches`를 수정해야 합니다:

```cpp
void UParticleSystemComponent::CollectMeshBatches(...)
{
    // 1. 인스턴스 데이터 수집
    TArray<FParticleInstanceData> InstanceData;
    for (FDynamicSpriteEmitterData* DynamicData : DynamicEmitterData)
    {
        const auto& Source = DynamicData->GetSource();
        for (int32 i = 0; i < Source.ActiveParticleCount; ++i)
        {
            int32 ParticleIndex = Source.DataContainer.ParticleIndices[i];
            FBaseParticle* Particle = GetParticle(Source, ParticleIndex);

            FParticleInstanceData Data;
            Data.FillFromParticle(Particle, GetWorldLocation());
            InstanceData.Add(Data);
        }
    }

    // 2. StructuredBuffer 업데이트
    D3D11RHI* RHI = GetRHI();
    RHI->UpdateStructuredBuffer(
        ParticleInstanceBuffer,
        InstanceData.data(),
        InstanceData.size() * sizeof(FParticleInstanceData)
    );

    // 3. 단일 FMeshBatchElement 생성
    FMeshBatchElement BatchElement;
    // ... 셰이더, 버퍼 설정 ...

    // 4. 인스턴싱 설정
    BatchElement.InstanceCount = InstanceData.size();
    BatchElement.ParticleInstanceSRV = ParticleInstanceSRV;  // t12에 바인딩

    // 5. 셰이더 매크로 설정
    TArray<FShaderMacro> Macros = ParticleMaterial->GetShaderMacros();
    Macros.Add({"PARTICLE_SPRITE", "1"});
    // 조명 모델도 설정
    Macros.Add({"LIGHTING_MODEL_PHONG", "1"});

    OutMeshBatchElements.Add(BatchElement);
}
```

### 7.4 DrawIndexedInstanced 호출

렌더러에서 `InstanceCount > 1`인 경우 `DrawIndexedInstanced` 사용:

```cpp
if (BatchElement.InstanceCount > 1)
{
    DeviceContext->DrawIndexedInstanced(
        BatchElement.IndexCount,
        BatchElement.InstanceCount,
        BatchElement.StartIndex,
        BatchElement.BaseVertexIndex,
        0  // StartInstanceLocation
    );
}
else
{
    DeviceContext->DrawIndexed(...);
}
```

### 7.5 셰이더 리소스 바인딩

`ParticleInstanceSRV`를 t12 슬롯에 바인딩:

```cpp
if (BatchElement.ParticleInstanceSRV)
{
    DeviceContext->VSSetShaderResources(12, 1, &BatchElement.ParticleInstanceSRV);
}
```

---

## 8. 향후 구현 과제

1. **메시/빔/리본 에미터**: 다양한 파티클 타입 지원
2. **고급 정렬 모드**: SortMode에 따른 다양한 정렬 방식
3. **LOD 시스템**: 거리에 따른 파티클 품질 조절
4. **GPU 기반 정렬**: Compute Shader를 사용한 정렬
