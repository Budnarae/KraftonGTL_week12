#pragma once

#include <cstdint>

/**
 * @class FParticleStatManager
 * @brief 파티클 렌더링과 관련된 통계 데이터를 수집하고 제공하는 싱글톤 클래스입니다.
 */
class FParticleStatManager
{
public:
	/**
	 * @brief FParticleStatManager의 싱글톤 인스턴스를 반환합니다.
	 */
	static FParticleStatManager& GetInstance()
	{
		static FParticleStatManager Instance;
		return Instance;
	}

	/**
	 * @brief 매 프레임 렌더링 시작 시 호출하여 프레임 단위 통계 데이터를 초기화합니다.
	 */
	void ResetFrameStats()
	{
		TotalParticleSystemCount = 0;
		ActiveParticleSystemCount = 0;
		TotalEmitterCount = 0;
		SpriteEmitterCount = 0;
		MeshEmitterCount = 0;
		RibbonEmitterCount = 0;
		BeamEmitterCount = 0;
		TotalParticleCount = 0;
		SpriteParticleCount = 0;
		MeshParticleCount = 0;
		RibbonParticleCount = 0;
		BeamParticleCount = 0;
		SpriteDrawCallCount = 0;
		MeshDrawCallCount = 0;
		RibbonDrawCallCount = 0;
		BeamDrawCallCount = 0;
		ParticlePassTimeMS = 0.0;
	}

	// --- Getters ---

	/** @return 씬 전체의 파티클 시스템 수 */
	uint32_t GetTotalParticleSystemCount() const { return TotalParticleSystemCount; }

	/** @return 활성화된 파티클 시스템 수 */
	uint32_t GetActiveParticleSystemCount() const { return ActiveParticleSystemCount; }

	/** @return 전체 에미터 수 */
	uint32_t GetTotalEmitterCount() const { return TotalEmitterCount; }

	/** @return 스프라이트 에미터 수 */
	uint32_t GetSpriteEmitterCount() const { return SpriteEmitterCount; }

	/** @return 메시 에미터 수 */
	uint32_t GetMeshEmitterCount() const { return MeshEmitterCount; }

	/** @return 리본 에미터 수 */
	uint32_t GetRibbonEmitterCount() const { return RibbonEmitterCount; }

	/** @return 빔 에미터 수 */
	uint32_t GetBeamEmitterCount() const { return BeamEmitterCount; }

	/** @return 전체 파티클 수 (Sprite + Mesh + Ribbon + Beam) */
	uint32_t GetTotalParticleCount() const { return TotalParticleCount; }

	/** @return 스프라이트 파티클 수 */
	uint32_t GetSpriteParticleCount() const { return SpriteParticleCount; }

	/** @return 메시 파티클 수 */
	uint32_t GetMeshParticleCount() const { return MeshParticleCount; }

	/** @return 리본 파티클 수 */
	uint32_t GetRibbonParticleCount() const { return RibbonParticleCount; }

	/** @return 빔 파티클 수 */
	uint32_t GetBeamParticleCount() const { return BeamParticleCount; }

	/** @return 스프라이트 파티클 드로우 콜 수 (GPU 인스턴싱 사용) */
	uint32_t GetSpriteDrawCallCount() const { return SpriteDrawCallCount; }

	/** @return 메시 파티클 드로우 콜 수 (개별 드로우) */
	uint32_t GetMeshDrawCallCount() const { return MeshDrawCallCount; }

	/** @return 리본 파티클 드로우 콜 수 */
	uint32_t GetRibbonDrawCallCount() const { return RibbonDrawCallCount; }

	/** @return 빔 파티클 드로우 콜 수 */
	uint32_t GetBeamDrawCallCount() const { return BeamDrawCallCount; }

	/** @return 파티클 패스 전체 소요 시간 (ms) */
	double GetParticlePassTimeMS() const { return ParticlePassTimeMS; }

	/**
	 * @brief 파티클 하나당 평균 렌더링 시간 (ms)을 계산하여 반환합니다.
	 * @return (전체 소요 시간) / (전체 파티클 수)
	 */
	double GetAverageTimePerParticleMS() const
	{
		if (TotalParticleCount == 0)
		{
			return 0.0;
		}
		return ParticlePassTimeMS / static_cast<double>(TotalParticleCount);
	}

	/**
	 * @brief 드로우 콜당 평균 렌더링 시간 (ms)을 계산하여 반환합니다.
	 * @return (전체 소요 시간) / (전체 드로우 콜 수)
	 */
	double GetAverageTimePerDrawCallMS() const
	{
		uint32_t TotalDrawCalls = SpriteDrawCallCount + MeshDrawCallCount + RibbonDrawCallCount + BeamDrawCallCount;
		if (TotalDrawCalls == 0)
		{
			return 0.0;
		}
		return ParticlePassTimeMS / static_cast<double>(TotalDrawCalls);
	}

	// --- Setters / Incrementers ---

	/** @brief 씬의 전체 파티클 시스템 수를 증가시킵니다 */
	void AddTotalParticleSystemCount(uint32_t InCount) { TotalParticleSystemCount += InCount; }

	/** @brief 활성화된 파티클 시스템 수를 증가시킵니다 */
	void AddActiveParticleSystemCount(uint32_t InCount) { ActiveParticleSystemCount += InCount; }

	/** @brief 전체 에미터 수를 증가시킵니다 */
	void AddTotalEmitterCount(uint32_t InCount) { TotalEmitterCount += InCount; }

	/** @brief 스프라이트 에미터 수를 1 증가시킵니다 */
	void IncrementSpriteEmitterCount() { ++SpriteEmitterCount; }

	/** @brief 메시 에미터 수를 1 증가시킵니다 */
	void IncrementMeshEmitterCount() { ++MeshEmitterCount; }

	/** @brief 리본 에미터 수를 1 증가시킵니다 */
	void IncrementRibbonEmitterCount() { ++RibbonEmitterCount; }

	/** @brief 빔 에미터 수를 1 증가시킵니다 */
	void IncrementBeamEmitterCount() { ++BeamEmitterCount; }

	/** @brief 스프라이트 파티클 수를 더합니다 */
	void AddSpriteParticleCount(uint32_t InCount)
	{
		SpriteParticleCount += InCount;
		TotalParticleCount += InCount;
	}

	/** @brief 메시 파티클 수를 더합니다 */
	void AddMeshParticleCount(uint32_t InCount)
	{
		MeshParticleCount += InCount;
		TotalParticleCount += InCount;
	}

	/** @brief 리본 파티클 수를 더합니다 */
	void AddRibbonParticleCount(uint32_t InCount)
	{
		RibbonParticleCount += InCount;
		TotalParticleCount += InCount;
	}

	/** @brief 빔 파티클 수를 더합니다 */
	void AddBeamParticleCount(uint32_t InCount)
	{
		BeamParticleCount += InCount;
		TotalParticleCount += InCount;
	}

	/** @brief 스프라이트 드로우 콜 수를 1 증가시킵니다 */
	void IncrementSpriteDrawCallCount() { ++SpriteDrawCallCount; }

	/** @brief 메시 드로우 콜 수를 1 증가시킵니다 */
	void IncrementMeshDrawCallCount() { ++MeshDrawCallCount; }

	/** @brief 리본 드로우 콜 수를 1 증가시킵니다 */
	void IncrementRibbonDrawCallCount() { ++RibbonDrawCallCount; }

	/** @brief 빔 드로우 콜 수를 1 증가시킵니다 */
	void IncrementBeamDrawCallCount() { ++BeamDrawCallCount; }

	/** @brief 파티클 패스의 전체 소요 시간을 직접 기록할 수 있도록 변수의 참조를 반환합니다. */
	double& GetParticlePassTimeSlot() { return ParticlePassTimeMS; }

private:
	FParticleStatManager() = default;
	~FParticleStatManager() = default;

	// 싱글톤 패턴을 위해 복사 및 대입을 금지합니다.
	FParticleStatManager(const FParticleStatManager&) = delete;
	FParticleStatManager& operator=(const FParticleStatManager&) = delete;

private:
	// --- 통계 데이터 멤버 변수 ---

	// 매 프레임 초기화되는 데이터
	uint32_t TotalParticleSystemCount = 0;    // 씬의 전체 파티클 시스템 수
	uint32_t ActiveParticleSystemCount = 0;   // 활성화된 파티클 시스템 수
	uint32_t TotalEmitterCount = 0;           // 전체 에미터 수
	uint32_t SpriteEmitterCount = 0;          // 스프라이트 에미터 수
	uint32_t MeshEmitterCount = 0;            // 메시 에미터 수
	uint32_t RibbonEmitterCount = 0;          // 리본 에미터 수
	uint32_t BeamEmitterCount = 0;            // 빔 에미터 수
	uint32_t TotalParticleCount = 0;          // 전체 파티클 수
	uint32_t SpriteParticleCount = 0;         // 스프라이트 파티클 수
	uint32_t MeshParticleCount = 0;           // 메시 파티클 수
	uint32_t RibbonParticleCount = 0;         // 리본 파티클 수
	uint32_t BeamParticleCount = 0;           // 빔 파티클 수
	uint32_t SpriteDrawCallCount = 0;         // 스프라이트 드로우 콜 수
	uint32_t MeshDrawCallCount = 0;           // 메시 드로우 콜 수
	uint32_t RibbonDrawCallCount = 0;         // 리본 드로우 콜 수
	uint32_t BeamDrawCallCount = 0;           // 빔 드로우 콜 수
	double ParticlePassTimeMS = 0.0;          // 파티클 패스 소요 시간 (ms)
};
