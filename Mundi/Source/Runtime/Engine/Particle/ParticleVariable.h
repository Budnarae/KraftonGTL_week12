#pragma once

// 컴파일 타임 상수 (배열 크기 등에 사용)
constexpr int32 MAX_PARTICLE_LODLEVEL_CONST = 8;
constexpr int32 MIN_PARTICLE_LODLEVEL_CONST = 0;

// 런타임 변수 (하위 호환성 유지)
extern int32 MIN_PARTICLE_LODLEVEL;
extern int32 MAX_PARTICLE_LODLEVEL;

/**
 * LOD 결정 방식을 정의하는 열거형 (언리얼 Cascade 방식 참조)
 *
 * @see https://docs.unrealengine.com/4.27/en-US/RenderingAndGraphics/ParticleSystems/LODs/
 */
enum class EParticleLODMethod : uint8
{
    /**
     * 자동 LOD 전환
     * - 매 LODDistanceCheckTime 초마다 카메라와의 거리를 체크
     * - LODDistances 배열을 참조하여 적절한 LOD 레벨 선택
     */
    Automatic,

    /**
     * 직접 설정
     * - 게임 코드에서 SetCurrentLODLevel()을 직접 호출하여 LOD 설정
     * - 자동 거리 체크 비활성화
     */
    DirectSet,

    /**
     * 활성화 시점 자동 설정
     * - Activate() 호출 시점과 루핑 시작 시점에만 거리 체크
     * - 그 외에는 DirectSet과 동일하게 동작
     */
    ActivateAutomatic
};