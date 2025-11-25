# 언리얼 엔진 파티클 시스템 구조체 가이드

## 개요

언리얼 엔진의 파티클 시스템은 **데이터**와 **리플레이 데이터**를 분리하여 관리합니다.

```
[메모리 관리]          [리플레이/직렬화]              [런타임 렌더링]
FParticleDataContainer → FDynamicEmitterReplayDataBase → FDynamicEmitterDataBase
                                    ↓                            ↓
                       FDynamicSpriteEmitterReplayDataBase  FDynamicSpriteEmitterDataBase
                                                                  ↓
                                                        FDynamicSpriteEmitterData
                                                                  ↓
                                                        FDynamicMeshEmitterData
```

---

## 1. FParticleDataContainer

**역할**: 파티클 데이터의 메모리 블록을 관리하는 컨테이너

### 멤버 변수

| 멤버 | 타입 | 설명 |
|------|------|------|
| `MemBlockSize` | int32 | 할당된 전체 메모리 블록 크기 (바이트) |
| `ParticleDataNumBytes` | int32 | 파티클 데이터가 차지하는 크기 |
| `ParticleIndicesNumShorts` | int32 | 인덱스 배열의 요소 개수 |
| `ParticleData` | uint8* | 메모리 블록 시작 주소 (파티클 데이터) |
| `ParticleIndices` | uint16* | 인덱스 배열 주소 (별도 할당 아님) |

### 메모리 레이아웃

```
┌─────────────────────────────────────────────────────┐
│                  할당된 메모리 블록                    │
├─────────────────────────────────┬───────────────────┤
│      ParticleData (AoS)         │  ParticleIndices  │
│   [Particle0][Particle1]...     │  [idx0][idx1]...  │
└─────────────────────────────────┴───────────────────┘
↑                                 ↑
ParticleData (malloc)      ParticleIndices (연산으로 계산)
```

### 핵심 포인트

- **단일 할당**: `ParticleData`만 malloc으로 할당
- **ParticleIndices는 별도 할당 없음**: `ParticleData + ParticleDataNumBytes` 위치를 가리킴
- **캐시 효율성**: 연속된 메모리로 캐시 히트율 향상

---

## 2. FDynamicEmitterReplayDataBase

**역할**: 에미터의 현재 상태를 **스냅샷**으로 저장하는 구조체

### "Replay"의 의미

- **네트워크 전송**: 서버 → 클라이언트로 파티클 상태 동기화
- **직렬화**: 저장/불러오기 시 파티클 상태 보존
- **디버깅**: 특정 프레임의 파티클 상태 재현

### 멤버 변수

| 멤버 | 타입 | 설명 |
|------|------|------|
| `eEmitterType` | EDynamicEmitterType | 에미터 종류 (Sprite, Mesh, Beam, Ribbon) |
| `ActiveParticleCount` | int32 | 현재 활성화된 파티클 수 |
| `ParticleStride` | int32 | 파티클 하나의 메모리 크기 |
| `DataContainer` | FParticleDataContainer | 파티클 데이터 컨테이너 |
| `Scale` | FVector3f | 에미터에 적용된 스케일 |
| `SortMode` | int32 | 파티클 정렬 방식 |

### 사용 흐름

```
[Tick] → 파티클 시뮬레이션 → [스냅샷 생성] → FDynamicEmitterReplayDataBase에 저장
                                    ↓
                            [네트워크 전송 / 저장]
                                    ↓
                            [수신 측에서 재현]
```

---

## 3. FDynamicSpriteEmitterReplayDataBase

**역할**: 스프라이트 에미터 전용 렌더링 정보를 추가로 저장

### 상속 관계

```cpp
struct FDynamicSpriteEmitterReplayDataBase : public FDynamicEmitterReplayDataBase
```

### 추가 멤버

| 멤버 | 타입 | 설명 |
|------|------|------|
| `MaterialInterface` | UMaterialInterface* | 파티클 렌더링에 사용할 머티리얼 |
| `RequiredModule` | FParticleRequiredModule* | 필수 모듈 정보 (LOD, 정렬 등) |

### 용도

- **렌더러에게 머티리얼 정보 전달**
- **스냅샷에 렌더링 설정 포함**

---

## 4. FDynamicEmitterDataBase

**역할**: 런타임 에미터 데이터의 **추상 베이스 클래스**

### 멤버 변수

| 멤버 | 타입 | 설명 |
|------|------|------|
| `EmitterIndex` | int32 | UParticleSystem::Emitters 배열에서의 인덱스 |

### 핵심 가상 함수

```cpp
virtual const FDynamicEmitterReplayDataBase& GetSource() const = 0;
```

- **순수 가상 함수**: 파생 클래스가 반드시 구현
- **역할**: 이 에미터의 리플레이 데이터(스냅샷)를 반환
- **다형성 활용**: 렌더러가 에미터 타입에 관계없이 데이터 접근 가능

### 사용 예시

```cpp
void Renderer::Draw(FDynamicEmitterDataBase* EmitterData)
{
    // 타입에 관계없이 스냅샷 데이터 획득
    const FDynamicEmitterReplayDataBase& Source = EmitterData->GetSource();

    // 공통 데이터 사용
    int32 ParticleCount = Source.ActiveParticleCount;
    uint8* Data = Source.DataContainer.ParticleData;
}
```

---

## 5. 렌더링 타입별 다형성 구조

### 상속 계층

```
FDynamicEmitterDataBase (추상)
        ↓
FDynamicSpriteEmitterDataBase (추상)
        ↓
FDynamicSpriteEmitterData (구체)
        ↓
FDynamicMeshEmitterData (구체)
```

### FDynamicSpriteEmitterDataBase

**역할**: 스프라이트 계열 에미터의 공통 기능

```cpp
struct FDynamicSpriteEmitterDataBase : public FDynamicEmitterDataBase
{
    void SortSpriteParticles(...);  // 파티클 정렬
    virtual int32 GetDynamicVertexStride(...) const = 0;  // Vertex 크기
};
```

### FDynamicSpriteEmitterData

**역할**: 일반 스프라이트(Billboard) 파티클

```cpp
virtual int32 GetDynamicVertexStride(ERHIFeatureLevel::Type InFeatureLevel) const override
{
    return sizeof(FParticleSpriteVertex);  // 스프라이트 전용 Vertex
}
```

### FDynamicMeshEmitterData

**역할**: 메시 파티클 (3D 모델을 파티클로 사용)

```cpp
virtual int32 GetDynamicVertexStride(ERHIFeatureLevel::Type /*InFeatureLevel*/) const override
{
    return sizeof(FMeshParticleInstanceVertex);  // 메시 인스턴싱용 Vertex
}
```

### GetDynamicVertexStride()의 다형성 활용

```cpp
void CreateVertexBuffer(FDynamicSpriteEmitterDataBase* EmitterData)
{
    // 타입에 따라 자동으로 올바른 Vertex 크기 사용
    int32 VertexSize = EmitterData->GetDynamicVertexStride(FeatureLevel);

    // Sprite → sizeof(FParticleSpriteVertex)
    // Mesh   → sizeof(FMeshParticleInstanceVertex)

    Buffer = new uint8[ParticleCount * VertexSize];
}
```

---

## 6. 전체 데이터 흐름

```
┌─────────────────┐
│ UParticleSystem │  (에셋 - 설계도)
└────────┬────────┘
         │ Activate()
         ↓
┌─────────────────────────┐
│ FParticleEmitterInstance │  (런타임 시뮬레이션)
│   - ParticleData[]       │
│   - ActiveParticles      │
└────────┬────────────────┘
         │ 매 프레임 스냅샷 생성
         ↓
┌────────────────────────────────┐
│ FDynamicEmitterReplayDataBase   │  (스냅샷/직렬화)
│   - DataContainer               │
│   - ActiveParticleCount         │
└────────┬───────────────────────┘
         │ GetSource()
         ↓
┌────────────────────────────┐
│ FDynamicSpriteEmitterData   │  (렌더링)
│   - GetDynamicVertexStride()│
│   - SortSpriteParticles()   │
└────────┬───────────────────┘
         │
         ↓
     [GPU 렌더링]
```

---

## 7. 시뮬레이션 → 렌더링 데이터 전달 과정 (상세)

시뮬레이션이 완료된 파티클 데이터가 렌더링 시스템으로 전달되는 과정을 단계별로 설명합니다.

---

### Step 1: 렌더 스레드용 Dynamic Data 생성 요청

**호출 위치**: `UParticleSystemComponent::TickComponent()` 또는 `SendRenderDynamicData_Concurrent()`

```cpp
// UParticleSystemComponent에서
void UParticleSystemComponent::TickComponent(float DeltaTime)
{
    // ... 시뮬레이션 완료 후 ...

    // 렌더 스레드에 전달할 데이터 생성 요청
    MarkRenderDynamicDataDirty();
}
```

**사용되는 멤버**:
- `UParticleSystemComponent::EmitterInstances` (`TArray<FParticleEmitterInstance*>`) - 각 에미터의 시뮬레이션 데이터

---

### Step 2: FDynamicSpriteEmitterData 생성

**호출 위치**: `FParticleEmitterInstance::GetDynamicData()`

각 에미터 인스턴스가 자신의 렌더링용 데이터 구조체를 생성합니다.

```cpp
FDynamicEmitterDataBase* FParticleEmitterInstance::GetDynamicData(bool bSelected)
{
    // 1. 렌더링용 데이터 구조체 생성
    FDynamicSpriteEmitterData* NewEmitterData = new FDynamicSpriteEmitterData(RequiredModule);

    // 2. 에미터 인덱스 설정
    NewEmitterData->EmitterIndex = EmitterIndex;

    return NewEmitterData;
}
```

**사용되는 멤버**:
- `FParticleEmitterInstance::EmitterIndex` (`int32`) - UParticleSystem::Emitters 배열에서의 위치

**생성되는 객체**:
- `FDynamicSpriteEmitterData` - 렌더링 타입별 구체 클래스

---

### Step 3: ReplayData에 스냅샷 복사

**호출 위치**: `FDynamicSpriteEmitterData::Init()` 또는 생성자

시뮬레이션 데이터를 렌더 스레드가 안전하게 읽을 수 있는 스냅샷으로 복사합니다.

```cpp
void FDynamicSpriteEmitterData::Init(FParticleEmitterInstance* EmitterInstance)
{
    // Source는 FDynamicSpriteEmitterReplayDataBase 타입
    FDynamicSpriteEmitterReplayDataBase* SourceData = &Source;

    // 3-1. 에미터 타입 설정
    SourceData->eEmitterType = EDynamicEmitterType::DET_Sprite;

    // 3-2. 활성 파티클 수 복사
    SourceData->ActiveParticleCount = EmitterInstance->ActiveParticles;

    // 3-3. 파티클 Stride 복사
    SourceData->ParticleStride = EmitterInstance->ParticleStride;

    // 3-4. 머티리얼 정보 복사
    SourceData->MaterialInterface = EmitterInstance->CurrentLODLevel->RequiredModule->Material;

    // 3-5. 스케일 복사
    SourceData->Scale = EmitterInstance->OwnerComponent->GetComponentScale();
}
```

**사용되는 멤버**:

| 소스 (FParticleEmitterInstance) | 대상 (FDynamicSpriteEmitterReplayDataBase) | 타입 |
|--------------------------------|-------------------------------------------|------|
| `ActiveParticles` | `ActiveParticleCount` | int32 |
| `ParticleStride` | `ParticleStride` | int32 |
| `CurrentLODLevel->RequiredModule->Material` | `MaterialInterface` | UMaterialInterface* |

---

### Step 4: ParticleDataContainer에 메모리 할당 및 복사

**호출 위치**: `FDynamicSpriteEmitterData::Init()` 내부

파티클 데이터와 인덱스 배열을 위한 메모리를 할당하고 복사합니다.

```cpp
void FDynamicSpriteEmitterData::Init(FParticleEmitterInstance* EmitterInstance)
{
    // ... Step 3 이후 ...

    FDynamicSpriteEmitterReplayDataBase* SourceData = &Source;
    FParticleDataContainer& DataContainer = SourceData->DataContainer;

    // 4-1. 필요한 메모리 크기 계산
    int32 ParticleDataSize = EmitterInstance->ActiveParticles * EmitterInstance->ParticleStride;
    int32 IndexDataSize = EmitterInstance->ActiveParticles * sizeof(uint16);

    DataContainer.ParticleDataNumBytes = ParticleDataSize;
    DataContainer.ParticleIndicesNumShorts = EmitterInstance->ActiveParticles;
    DataContainer.MemBlockSize = ParticleDataSize + IndexDataSize;

    // 4-2. 메모리 할당 (파티클 데이터 + 인덱스를 한 번에)
    DataContainer.ParticleData = (uint8*)FMemory::Malloc(DataContainer.MemBlockSize);

    // 4-3. 인덱스 포인터 설정 (ParticleData 끝에 위치)
    DataContainer.ParticleIndices = (uint16*)(DataContainer.ParticleData + ParticleDataSize);

    // 4-4. 파티클 데이터 복사 (시뮬레이션 → 렌더링)
    FMemory::Memcpy(
        DataContainer.ParticleData,                    // 대상
        EmitterInstance->ParticleData,                 // 소스
        ParticleDataSize                               // 크기
    );

    // 4-5. 인덱스 배열 생성 (정렬 전 기본 순서)
    for (int32 i = 0; i < EmitterInstance->ActiveParticles; ++i)
    {
        DataContainer.ParticleIndices[i] = (uint16)i;
    }
}
```

**사용되는 멤버**:

| 멤버 | 타입 | 역할 |
|------|------|------|
| `FParticleDataContainer::MemBlockSize` | int32 | 할당할 총 메모리 크기 |
| `FParticleDataContainer::ParticleDataNumBytes` | int32 | 파티클 데이터 영역 크기 |
| `FParticleDataContainer::ParticleIndicesNumShorts` | int32 | 인덱스 배열 요소 수 |
| `FParticleDataContainer::ParticleData` | uint8* | 파티클 데이터 시작 주소 |
| `FParticleDataContainer::ParticleIndices` | uint16* | 인덱스 배열 시작 주소 |

**핵심 포인트**:
- **단일 할당**: `ParticleData`만 malloc하고, `ParticleIndices`는 그 끝에 위치
- **깊은 복사**: 시뮬레이션 데이터를 렌더 스레드용으로 완전히 복사
- **스레드 안전성**: 복사본이므로 게임 스레드와 렌더 스레드가 동시 접근 가능

---

### Step 5: 파티클 정렬 (선택적)

**호출 위치**: `FDynamicSpriteEmitterDataBase::SortSpriteParticles()`

카메라와의 거리에 따라 파티클을 정렬합니다 (반투명 렌더링을 위해).

```cpp
void FDynamicSpriteEmitterDataBase::SortSpriteParticles(
    int32 SortMode,
    bool bLocalSpace,
    FVector CameraPosition,
    FMatrix LocalToWorld
)
{
    FDynamicSpriteEmitterReplayDataBase* SourceData = GetSourceData();
    FParticleDataContainer& DataContainer = SourceData->DataContainer;

    // 5-1. 각 파티클의 정렬 키 계산 (카메라 거리)
    TArray<float> SortKeys;
    SortKeys.SetNum(SourceData->ActiveParticleCount);

    for (int32 i = 0; i < SourceData->ActiveParticleCount; ++i)
    {
        // 파티클 데이터 접근
        uint8* ParticleBase = DataContainer.ParticleData + i * SourceData->ParticleStride;
        FBaseParticle* Particle = (FBaseParticle*)ParticleBase;

        // 카메라와의 거리 계산
        FVector WorldPos = bLocalSpace ? LocalToWorld.TransformPosition(Particle->Location) : Particle->Location;
        SortKeys[i] = FVector::DistSquared(WorldPos, CameraPosition);
    }

    // 5-2. 인덱스 배열을 정렬 키 기준으로 정렬
    // ParticleIndices를 정렬하여 실제 데이터는 이동하지 않음
    Sort(DataContainer.ParticleIndices, SourceData->ActiveParticleCount,
        [&SortKeys](uint16 A, uint16 B) {
            return SortKeys[A] > SortKeys[B];  // 먼 것부터 (Back-to-Front)
        }
    );
}
```

**사용되는 멤버**:
- `FDynamicEmitterReplayDataBase::SortMode` (`int32`) - 정렬 방식 (None, ViewDepth, Distance 등)
- `FParticleDataContainer::ParticleIndices` (`uint16*`) - 정렬된 접근 순서

**핵심 포인트**:
- **인덱스 정렬**: 파티클 데이터를 이동하지 않고 인덱스만 재배열
- **캐시 효율**: 데이터 이동 없이 정렬 순서 변경

---

### Step 6: SceneProxy가 DynamicData 배열을 순회

**호출 위치**: `FParticleSystemSceneProxy::GetDynamicMeshElements()`

렌더러(SceneProxy)가 각 에미터의 DynamicData를 순회하며, 타입별 렌더링 함수를 호출합니다.

```cpp
void FParticleSystemSceneProxy::GetDynamicMeshElements(
    const TArray<const FSceneView*>& Views,
    const FSceneViewFamily& ViewFamily,
    FMeshElementCollector& Collector
)
{
    // 6-1. 각 에미터의 DynamicData를 순회
    for (int32 EmitterIndex = 0; EmitterIndex < DynamicEmitterDataArray.Num(); ++EmitterIndex)
    {
        FDynamicEmitterDataBase* EmitterData = DynamicEmitterDataArray[EmitterIndex];
        if (!EmitterData) continue;

        // 6-2. 에미터 타입에 따라 적절한 렌더링 함수 호출
        // FDynamicEmitterDataBase는 추상 클래스이므로,
        // 실제로는 FDynamicSpriteEmitterData 등의 구체 클래스

        // 타입 확인을 위해 GetSource() 사용
        const FDynamicEmitterReplayDataBase& Source = EmitterData->GetSource();

        if (Source.eEmitterType == DET_Sprite)
        {
            // 6-3. 스프라이트 타입으로 다운캐스팅
            FDynamicSpriteEmitterData* SpriteEmitterData =
                static_cast<FDynamicSpriteEmitterData*>(EmitterData);

            // 6-4. 각 View에 대해 렌더링 요소 생성 요청
            for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
            {
                // ★ 여기서 실제 Vertex 버퍼 생성 함수 호출 ★
                SpriteEmitterData->GetDynamicMeshElementsEmitter(
                    Views[ViewIndex],
                    ViewFamily,
                    ViewIndex,
                    Collector,
                    this  // SceneProxy
                );
            }
        }
        else if (Source.eEmitterType == DET_Mesh)
        {
            FDynamicMeshEmitterData* MeshEmitterData =
                static_cast<FDynamicMeshEmitterData*>(EmitterData);

            // 메시 타입 렌더링...
        }
    }
}
```

**핵심 포인트**:
- `GetSource()`는 **타입 확인 및 공통 데이터 접근용**
- 실제 렌더링은 **구체 클래스의 가상 함수**(`GetDynamicMeshElementsEmitter`)가 담당
- SceneProxy는 **오케스트레이터** 역할만 수행

**호출 계층 구조**:
```
FParticleSystemSceneProxy::GetDynamicMeshElements()
    │
    ├─ EmitterData->GetSource()  // 타입 확인용
    │
    └─ SpriteEmitterData->GetDynamicMeshElementsEmitter()  // 실제 렌더링
           │
           ├─ GetSource()로 ReplayData 접근
           ├─ GetDynamicVertexStride()로 Vertex 크기
           └─ Vertex 버퍼 생성 및 Draw Call
```

**사용되는 멤버**:
- `FParticleSystemSceneProxy::DynamicEmitterDataArray` (`TArray<FDynamicEmitterDataBase*>`)
- `FDynamicEmitterReplayDataBase::eEmitterType` (`EDynamicEmitterType`)

---

### Step 7: GetDynamicMeshElementsEmitter 내부에서 GetSource() 호출

**호출 위치**: `FDynamicSpriteEmitterData::GetDynamicMeshElementsEmitter()` 시작 부분

구체 클래스의 렌더링 함수 내부에서 자신의 ReplayData에 접근합니다.

```cpp
void FDynamicSpriteEmitterData::GetDynamicMeshElementsEmitter(
    const FSceneView* View,
    const FSceneViewFamily& ViewFamily,
    int32 ViewIndex,
    FMeshElementCollector& Collector,
    FParticleSystemSceneProxy* SceneProxy
)
{
    // 7-1. 자신의 ReplayData 획득
    // GetSource()는 FDynamicSpriteEmitterData가 소유한
    // FDynamicSpriteEmitterReplayDataBase를 반환
    const FDynamicSpriteEmitterReplayDataBase& SourceData =
        static_cast<const FDynamicSpriteEmitterReplayDataBase&>(GetSource());

    // 7-2. 유효성 검사
    if (SourceData.ActiveParticleCount == 0)
    {
        return;
    }

    // 7-3. 공통 데이터 접근
    int32 ParticleCount = SourceData.ActiveParticleCount;
    int32 Stride = SourceData.ParticleStride;
    uint8* ParticleData = SourceData.DataContainer.ParticleData;
    uint16* ParticleIndices = SourceData.DataContainer.ParticleIndices;

    // 7-4. 스프라이트 전용 데이터 접근
    UMaterialInterface* Material = SourceData.MaterialInterface;

    // ... Step 8에서 Vertex 버퍼 생성 ...
}
```

**GetSource()의 구현** (FDynamicSpriteEmitterData 내부):

```cpp
struct FDynamicSpriteEmitterData : public FDynamicSpriteEmitterDataBase
{
private:
    // 이 에미터의 ReplayData를 멤버로 소유
    FDynamicSpriteEmitterReplayDataBase Source;

public:
    // 순수 가상 함수 구현 - 자신의 Source 반환
    virtual const FDynamicEmitterReplayDataBase& GetSource() const override
    {
        return Source;
    }
};
```

**핵심 포인트**:
- `FDynamicSpriteEmitterData`는 `FDynamicSpriteEmitterReplayDataBase`를 **멤버로 소유**
- `GetSource()`는 그 멤버의 참조를 반환
- 렌더링 함수 내부에서 자신의 데이터에 접근하기 위해 `GetSource()` 호출

**데이터 소유 관계**:
```
FDynamicSpriteEmitterData
├─ EmitterIndex (int32)
├─ Source (FDynamicSpriteEmitterReplayDataBase)  ← GetSource()가 반환
│   ├─ ActiveParticleCount (int32)
│   ├─ ParticleStride (int32)
│   ├─ DataContainer (FParticleDataContainer)
│   │   ├─ ParticleData (uint8*)
│   │   └─ ParticleIndices (uint16*)
│   ├─ MaterialInterface (UMaterialInterface*)
│   └─ Scale (FVector3f)
└─ (기타 렌더링 상태)
```

---

### Step 8: Vertex 버퍼 생성

**호출 위치**: `FDynamicSpriteEmitterData::GetDynamicMeshElementsEmitter()` 중반부

파티클 데이터를 GPU가 이해하는 Vertex 형식으로 변환합니다.

```cpp
void FDynamicSpriteEmitterData::GetDynamicMeshElementsEmitter(
    const FSceneView* View,
    const FSceneViewFamily& ViewFamily,
    int32 ViewIndex,
    FMeshElementCollector& Collector,
    FParticleSystemSceneProxy* SceneProxy
)
{
    const FDynamicSpriteEmitterReplayDataBase& SourceData =
        static_cast<const FDynamicSpriteEmitterReplayDataBase&>(GetSource());

    // 7-1. Vertex Stride 획득 (다형성)
    int32 VertexStride = GetDynamicVertexStride(View->GetFeatureLevel());
    // Sprite: sizeof(FParticleSpriteVertex)
    // Mesh: sizeof(FMeshParticleInstanceVertex)

    // 7-2. Vertex 버퍼 메모리 할당
    int32 VertexCount = SourceData.ActiveParticleCount * 4;  // 쿼드당 4 vertices
    FGlobalDynamicVertexBuffer::FAllocation Allocation =
        Collector.AllocateOneFrameResource<FVertexBuffer>(VertexCount * VertexStride);

    FParticleSpriteVertex* Vertices = (FParticleSpriteVertex*)Allocation.Buffer;

    // 7-3. 정렬된 순서로 파티클 순회
    for (int32 i = 0; i < SourceData.ActiveParticleCount; ++i)
    {
        // 인덱스 배열을 통해 정렬된 순서로 접근
        uint16 ParticleIndex = SourceData.DataContainer.ParticleIndices[i];

        // 파티클 데이터 주소 계산
        uint8* ParticleBase = SourceData.DataContainer.ParticleData +
                              ParticleIndex * SourceData.ParticleStride;
        FBaseParticle* Particle = (FBaseParticle*)ParticleBase;

        // 7-4. FBaseParticle → FParticleSpriteVertex 변환
        int32 VertexIndex = i * 4;

        // 쿼드의 4개 버텍스 생성 (Billboard)
        for (int32 Corner = 0; Corner < 4; ++Corner)
        {
            FParticleSpriteVertex& Vertex = Vertices[VertexIndex + Corner];

            Vertex.Position = Particle->Location;
            Vertex.Size = FVector2D(Particle->Size.X, Particle->Size.Y);
            Vertex.Color = Particle->Color;
            Vertex.Rotation = Particle->Rotation;

            // UV 좌표 (코너별로 다름)
            static const FVector2D CornerUVs[4] = {
                FVector2D(0, 0), FVector2D(1, 0),
                FVector2D(1, 1), FVector2D(0, 1)
            };
            Vertex.UV = CornerUVs[Corner];
        }
    }
}
```

**사용되는 함수**:
- `FDynamicSpriteEmitterData::GetDynamicVertexStride()` - Vertex 크기 반환

**사용되는 멤버 (FBaseParticle)**:

| 멤버 | 타입 | Vertex에서의 용도 |
|------|------|------------------|
| `Location` | FVector | 파티클 중심 위치 |
| `Size` | FVector | 쿼드 크기 (X=Width, Y=Height) |
| `Color` | FLinearColor | 파티클 색상 |
| `Rotation` | float | 빌보드 회전 각도 |

**핵심 포인트**:
- **정렬된 순서 사용**: `ParticleIndices`를 통해 접근
- **FBaseParticle만 사용**: 렌더러는 Payload에 접근하지 않음

---

### Step 9: Draw Call 생성 및 GPU 제출

**호출 위치**: `FDynamicSpriteEmitterData::GetDynamicMeshElementsEmitter()` 후반부

```cpp
void FDynamicSpriteEmitterData::GetDynamicMeshElementsEmitter(...)
{
    // ... Step 7 이후 ...

    // 8-1. 메시 배치 요소 생성
    FMeshBatch& MeshBatch = Collector.AllocateMesh();

    // 8-2. 머티리얼 설정
    const FDynamicSpriteEmitterReplayDataBase& SourceData =
        static_cast<const FDynamicSpriteEmitterReplayDataBase&>(GetSource());
    MeshBatch.MaterialRenderProxy = SourceData.MaterialInterface->GetRenderProxy();

    // 8-3. Vertex/Index 버퍼 설정
    MeshBatch.VertexFactory = &ParticleSpriteVertexFactory;

    FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
    BatchElement.FirstIndex = 0;
    BatchElement.NumPrimitives = SourceData.ActiveParticleCount * 2;  // 쿼드당 2 triangles
    BatchElement.MinVertexIndex = 0;
    BatchElement.MaxVertexIndex = SourceData.ActiveParticleCount * 4 - 1;

    // 8-4. Collector에 추가 (렌더 커맨드로 변환됨)
    Collector.AddMesh(ViewIndex, MeshBatch);
}
```

**사용되는 멤버**:
- `FDynamicSpriteEmitterReplayDataBase::MaterialInterface` (`UMaterialInterface*`)
- `FDynamicEmitterReplayDataBase::ActiveParticleCount` (`int32`)

---

### 전체 흐름 요약 다이어그램

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    게임 스레드 (Tick)                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Step 1: TickComponent() 완료                                           │
│     │                                                                   │
│     ↓                                                                   │
│  Step 2: GetDynamicData() → FDynamicSpriteEmitterData 생성              │
│     │    └─ EmitterIndex 설정                                           │
│     ↓                                                                   │
│  Step 3: Init() → ReplayData(Source 멤버)에 스냅샷 복사                  │
│     │    ├─ ActiveParticleCount                                         │
│     │    ├─ ParticleStride                                              │
│     │    └─ MaterialInterface                                           │
│     ↓                                                                   │
│  Step 4: DataContainer에 메모리 할당 및 복사                             │
│     │    ├─ ParticleData = malloc(MemBlockSize)                         │
│     │    ├─ ParticleIndices = ParticleData + ParticleDataNumBytes       │
│     │    └─ Memcpy(시뮬레이션 데이터)                                    │
│     ↓                                                                   │
│  ═══════════════════════════════════════════════════════════════════    │
│                     렌더 스레드로 전달                                    │
│  ═══════════════════════════════════════════════════════════════════    │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                    렌더 스레드 (Draw)                                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Step 5: SortSpriteParticles() → ParticleIndices 정렬                   │
│     │    └─ 카메라 거리 기반 Back-to-Front                               │
│     ↓                                                                   │
│  Step 6: SceneProxy::GetDynamicMeshElements()                           │
│     │    ├─ DynamicEmitterDataArray 순회                                │
│     │    ├─ GetSource()로 eEmitterType 확인                             │
│     │    └─ 타입별 GetDynamicMeshElementsEmitter() 호출                  │
│     ↓                                                                   │
│  Step 7: GetDynamicMeshElementsEmitter() 내부                           │
│     │    └─ GetSource()로 자신의 ReplayData(Source 멤버) 접근            │
│     ↓                                                                   │
│  Step 8: Vertex 버퍼 생성                                               │
│     │    ├─ GetDynamicVertexStride() → Vertex 크기                      │
│     │    ├─ ParticleIndices 순서로 순회                                  │
│     │    └─ FBaseParticle → FParticleSpriteVertex 변환                  │
│     ↓                                                                   │
│  Step 9: Draw Call 생성                                                 │
│     │    ├─ MaterialRenderProxy 설정                                    │
│     │    ├─ NumPrimitives = ActiveParticleCount * 2                     │
│     │    └─ Collector.AddMesh()                                         │
│     ↓                                                                   │
│  [GPU 렌더링]                                                           │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 8. 요약 비교표

| 구조체 | 역할 | 할당 | 용도 |
|--------|------|------|------|
| `FParticleDataContainer` | 메모리 관리 | malloc 1회 | 파티클 데이터 + 인덱스 저장 |
| `FDynamicEmitterReplayDataBase` | 스냅샷 | 값 타입 | 직렬화, 네트워크, 디버깅 |
| `FDynamicSpriteEmitterReplayDataBase` | 스프라이트 스냅샷 | 값 타입 | 머티리얼 정보 포함 |
| `FDynamicEmitterDataBase` | 런타임 베이스 | 추상 | GetSource() 다형성 |
| `FDynamicSpriteEmitterData` | 스프라이트 렌더링 | 구체 | Billboard Vertex |
| `FDynamicMeshEmitterData` | 메시 렌더링 | 구체 | Instance Vertex |
