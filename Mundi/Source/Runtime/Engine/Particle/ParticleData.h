#pragma once
#include "ParticleEmitter.h"

struct FBaseParticle
{
public:
    // -------------------------------------------
    // 1. 공간 및 운동 (Spatial and Kinematics)
    // -------------------------------------------
    
    // 16 bytes
    FVector		OldLocation{};			// Last frame's location, used for collision
    // 파티클의 회전 속도 (매 프레임 Rotation에 더해짐)
    float		BaseRotationRate{};		// Initial angular velocity of particle (in Radians per second)

    // 16 bytes
    FVector     Location{};             // 현재 파티클의 월드 공간 위치 (Update 함수에서 Velocity를 이용해 매 프레임 갱신됨)
    float		Rotation{};				// 파티클의 현재 회전 각도 (스프라이트 회전에 사용)

    // 16 bytes
    FVector     BaseVelocity{};         // 파티클이 처음 생성될 때 설정된 초기 속도 (재계산 방지 또는 참조용)
    float	    RotationRate{};			// Current rotation rate, gets reset to BaseRotationRate each frame

    // 16 bytes
    FVector     Velocity{};             // 현재 파티클의 속도 벡터 (Update 함수에서 Gravity, Drag 등에 의해 갱신됨)
    float       RelativeTime{};         // 파티클의 전체 수명 (Lifetime) 중 경과된 시간의 비율 (0.0 ~ 1.0). ColorOverLife, SizeOverLife 등의 모듈에서 보간(Interpolation)을 위해 사용됨.

    // 16 bytes
    FVector		BaseSize{};				// Size = BaseSize at the start of each frame
    float       LifeTime{};             // 파티클의 총 수명

    // 16 bytes
    FVector		Size{};					// Current size, gets reset to BaseSize each frame
    int32       Flags{};				// Flags indicating various particle states
    
    // 16 bytes
    FLinearColor	Color{};			// Current color of particle.

    // 16 bytes
    FLinearColor	BaseColor{};		// Base color of the particle

    // 16 bytes
    float	    OneOverMaxLifetime{};	// Reciprocal of lifetime
    float       Padding[3];
    
    // -------------------------------------------
    // 4. ... (선택적 페이로드 메모리 시작 지점)
    // -------------------------------------------
    // 이 뒤에 Payload 및 Padding 영역이 이어지지만, 구조체 선언에는 포함되지 않습니다.
};

// 전방 선언
class UParticleSystemComponent;

// 파티클 모듈에 전달되는 컨텍스트 정보
struct FParticleContext
{
    // 현재 처리 중인 파티클 데이터
    FBaseParticle* Particle;

    // 이 파티클을 소유하는 컴포넌트
    UParticleSystemComponent* Owner;

    // 파티클 인덱스 (이벤트 생성용)
    int32 ParticleIndex;

    FParticleContext(FBaseParticle* InParticle, UParticleSystemComponent* InOwner, int32 InParticleIndex = -1)
        : Particle(InParticle)
        , Owner(InOwner)
        , ParticleIndex(InParticleIndex)
    {}
};

// FParticleDataContainer는 파티클 시스템의 런타임 메모리 블록을 관리합니다.
struct FParticleDataContainer
{
public:
    // -------------------------------------------
    // 1. 데이터 멤버 (Core Data Members)
    // -------------------------------------------

    // 할당된 전체 메모리 블록의 총 크기 (바이트). (Base 데이터 + Indices 공간 포함)
    int32 MemBlockSize; 

    // 모든 파티클의 실제 데이터(AoS)가 차지하는 공간 크기 (바이트).
    int32 ParticleDataNumBytes; 

    // ParticleIndices 배열에 저장된 uint16(2바이트) 요소의 개수 (활성 파티클 개수)
    int32 ParticleIndicesNumShorts;

    // 할당된 메모리 블록의 시작 주소. 파티클 데이터 배열의 시작점과 동일합니다.
    uint8* ParticleData; 

    // [핵심] ParticleData 블록의 끝에 위치하는 인덱스 배열의 포인터.
    // ParticleData 뒤에 메모리가 이어지므로 별도 할당(new/malloc)이 필요 없습니다.
    uint16* ParticleIndices; 

public:
    // -------------------------------------------
    // 2. 핵심 인터페이스 (Core Functions)
    // -------------------------------------------

    // 필요한 총 메모리 크기를 계산하여 할당하고 ParticleData 및 ParticleIndices 포인터를 설정합니다.
    void AllocateMemory(int32 MaxParticleCount, int32 ParticleStride);

    // 할당된 메모리를 해제하고 포인터를 초기화합니다.
    void FreeMemory();

    // ParticleData 포인터를 기반으로 인덱스 배열의 시작 주소를 계산하여 ParticleIndices에 설정합니다.
    void SetupIndicesPtr();
};

// 에미터 타입 정의
enum EDynamicEmitterType : int32
{
    EDET_Sprite,
    EDET_Mesh,
    EDET_Beam,
    EDET_Ribbon
};

struct FDynamicEmitterReplayDataBase
{
public:
    // -------------------------------------------
    // 1. 최소 데이터 멤버 (Core Data Members)
    // -------------------------------------------

    // 에미터의 종류를 나타내는 타입
    EDynamicEmitterType eEmitterType;

    // 스냅샷이 찍힌 시점에 활성화된 파티클의 개수
    int32 ActiveParticleCount;

    // 파티클 하나당 메모리 크기 (Base + Payload + Padding)
    // 수신 측에서 DataContainer의 바이트를 해석하는 데 필수적임
    int32 ParticleStride; 
    
    // 파티클 데이터 및 인덱스 배열의 메모리 관리 구조체
    // (여기에는 raw data 포인터와 크기가 포함됨)
    FParticleDataContainer DataContainer;

    // 이 에미터에 적용된 스케일 값
    FVector Scale;

    // 파티클 렌더링 시 사용되는 정렬 방식
    int32 SortMode; 

public:
    // -------------------------------------------
    // 2. 핵심 인터페이스 (Core Functions - Serialization)
    // -------------------------------------------
    
    // 이 구조체의 내용을 직렬화/역직렬화하는 함수 (네트워크 전송이나 저장/불러오기에 사용)
    // (FArchive는 Unreal의 직렬화 스트림 클래스라고 가정)
    bool Serialize(FArchive& Ar); 

    // 이 데이터가 재현 가능한 유효한 상태인지 검증합니다.
    bool IsValid() const;
};

// 전방 선언
struct FParticleRequiredModule; 

// 스프라이트 에미터의 시각적 재생 상태를 위한 데이터 구조체
struct FDynamicSpriteEmitterReplayDataBase : public FDynamicEmitterReplayDataBase
{
public:
    // -------------------------------------------
    // 1. 핵심 데이터 멤버 (Rendering Specific)
    // -------------------------------------------

    // 이 스프라이트 에미터가 사용하는 재질 템플릿
    UMaterial* MaterialInterface;

    // 파티클의 렌더링 방식(정렬, LOD 등)을 정의하는 필수 모듈 데이터 포인터
    FParticleRequiredModule* RequiredModule;

public:
    // -------------------------------------------
    // 2. 생성자/소멸자 (Minimal)
    // -------------------------------------------
    
    FDynamicSpriteEmitterReplayDataBase() = default;
    virtual ~FDynamicSpriteEmitterReplayDataBase() = default;

    // -------------------------------------------
    // 3. 인터페이스 (Inherited/Override)
    // -------------------------------------------

    // 상위 클래스에서 상속받은 Serialize 함수를 재정의하여 
    // MaterialInterface와 RequiredModule을 직렬화합니다.
    // virtual bool Serialize(FArchive& Ar) override; 
};

class UParticleEmitter;
struct FDynamicEmitterDataBase
{
public:
    // -------------------------------------------
    // 1. 최소 데이터 멤버 (Core Data Members)
    // -------------------------------------------

    // UParticleSystem::Emitters 배열 내에서 이 인스턴스의 템플릿이 위치한 인덱스
    int32 EmitterIndex;

    // 이 인스턴스를 정의하는 템플릿 에미터(UParticleEmitter)에 대한 포인터
    UParticleEmitter* SourceEmitter; 
    
    // 이 인스턴스가 생성된 시점의 시간
    float SpawnTime;

public:
    // -------------------------------------------
    // 2. 순수 가상 핵심 인터페이스 (Pure Virtual Core Interface)
    // -------------------------------------------

    // **[핵심]** 이 에미터의 현재 상태를 직렬화(Serialization) 데이터 구조체로 반환합니다.
    // '= 0' 이 붙어 이 함수는 반드시 파생 클래스에서 구현되어야 합니다.
    virtual const FDynamicEmitterReplayDataBase& GetSource() const = 0;

    // 에미터 인스턴스의 시뮬레이션 메인 루프 (파티클 생성 및 업데이트 관리)
    virtual void Tick(float DeltaTime) = 0;
    
    // 이 에미터 인스턴스가 할당한 모든 메모리 및 리소스를 해제합니다.
    virtual void FreeData() = 0;

    // 파생 클래스의 메모리 누수 방지를 위한 가상 소멸자 (상속 시 필수)
    virtual ~FDynamicEmitterDataBase() {}
};

// 외부 UObject 클래스들에 대한 전방 선언
class UParticleLODLevel;
struct FParticleEventInstancePayload;
enum class ERHIFeatureLevel : int32; // 렌더링 피처 레벨 정의

// 렌더링에 필요한 Vertex 구조체
struct FParticleSpriteVertex
{
    FVector WorldPosition;  // 파티클 월드 위치
    FVector2D UV;           // 텍스처 좌표 (0~1)
    FVector2D Size;         // 파티클 크기 (Width, Height)
    FLinearColor Color;     // 파티클 색상
    float Rotation;         // 회전 각도 (Radians)
    float Padding[3];       // 16바이트 정렬을 위한 패딩

    // FBaseParticle로부터 데이터 채우기
    void FillFromParticle(const FBaseParticle* Particle, const FVector2D& InUV)
    {
        WorldPosition = Particle->Location;
        UV = InUV;
        Size = FVector2D(Particle->Size.X, Particle->Size.Y);
        Color = Particle->Color;
        Rotation = Particle->Rotation;

        // 패딩 초기화
        Padding[0] = 0.0f;
        Padding[1] = 0.0f;
        Padding[2] = 0.0f;
    }
};

// GPU 인스턴싱용 파티클 인스턴스 데이터 (셰이더와 일치해야 함)
// HLSL StructuredBuffer와 정확히 일치하도록 패킹 지정
#pragma pack(push, 4)
struct FParticleInstanceData
{
    FVector Position;       // 12 bytes (float3 in HLSL)
    float Rotation;         // 4 bytes  -> 16 bytes total
    FVector2D Size;         // 8 bytes (float2 in HLSL)
    FVector2D Padding;      // 8 bytes  -> 16 bytes total
    FLinearColor Color;     // 16 bytes (float4 in HLSL)
    // Total: 48 bytes

    // FBaseParticle로부터 데이터 채우기
    void FillFromParticle(const FBaseParticle* Particle, const FVector& ComponentLocation)
    {
        Position = Particle->Location + ComponentLocation;
        Rotation = Particle->Rotation;
        Size = FVector2D(Particle->Size.X, Particle->Size.Y);
        Padding = FVector2D(0.0f, 0.0f);
        Color = Particle->Color;
    }
};
#pragma pack(pop)

struct FMeshParticleInstanceVertex;

// ----------------------------------------------------
// [A] 렌더링 타입별 다형성 데이터 (Dynamic Data Hierarchy)
// ----------------------------------------------------

// 전방 선언
struct FParticleEmitterInstance;

// ============================================================================
// FDynamicSpriteEmitterData
// ============================================================================
// 스프라이트 에미터의 렌더링용 데이터 구조체입니다.
// FParticleEmitterInstance로부터 렌더링에 필요한 정보를 추출하여 보관합니다.
// 싱글 스레드 환경에서는 시뮬레이션 데이터를 직접 참조합니다.
// ============================================================================
struct FDynamicSpriteEmitterData
{
    FDynamicSpriteEmitterData() = default;
    ~FDynamicSpriteEmitterData();

    // UParticleSystem::Emitters 배열에서의 인덱스
    int32 EmitterIndex = 0;

    // 렌더링용 스냅샷 데이터
    FDynamicSpriteEmitterReplayDataBase Source;

    // ========================================================================
    // 초기화 및 업데이트
    // ========================================================================

    // EmitterInstance로부터 렌더링 데이터 초기화/갱신
    void Init(FParticleEmitterInstance* Instance, int32 Index);

    // ========================================================================
    // 파티클 정렬
    // ========================================================================

    // 카메라 위치 기준으로 파티클을 Back-to-Front 정렬
    void SortParticles(const FVector& CameraPosition);

    // ========================================================================
    // 데이터 접근
    // ========================================================================

    // 스냅샷 데이터 반환
    const FDynamicSpriteEmitterReplayDataBase& GetSource() const { return Source; }

    // Vertex Stride 반환
    int32 GetDynamicVertexStride() const { return sizeof(FParticleSpriteVertex); }

    // ========================================================================
    // 메모리 관리
    // ========================================================================

    // 할당된 인덱스 배열 해제
    void Release();
};

// ----------------------------------------------------
// [B] 런타임 에미터 코어 (FParticleEmitterInstance)
// ----------------------------------------------------

