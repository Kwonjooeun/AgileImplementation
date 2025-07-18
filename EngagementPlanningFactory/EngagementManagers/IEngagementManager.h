#pragma once

#include "../../dds_message/AIEP_AIEP_.hpp"
#include <memory>
#include <vector>
#include <chrono>


namespace MINEASMALM {
	
	// 무장 할당 정보
	struct WeaponAssignmentInfo {
	int tubeNumber;
	EN_WPN_KIND weaponKind;
	
	// 표적 정보 (optional)
	std::optional<SGEODETIC_POSITION> targetPosition;
	std::optional<uint32_t> systemTargetId;
	
	// 자항기뢰 정보 (optional)
	std::optional<uint32_t> dropPlanListNumber;
	std::optional<uint32_t> dropPlanNumber;
	
	// 공통 정보
	
	uint32_t requestConsole;
	uint32_t allocConsole;
	};
	
	// =============================================================================
	// 교전계획 관리자 인터페이스 계층
	// =============================================================================
	
	class IEngagementManager {
	public:
		virtual ~IEngagementManager() = default;
		virtual void Reset() = 0;
		virtual void Shutdown() = 0;
		virtual void WorkerLoop() = 0;
		virtual void UpdateOwnShipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownShip) = 0;
		virtual void UpdateTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& target) = 0;
		virtual void UpdateWaypoints(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& waypoints) = 0;
		
		virtual ST_3D_GEODETIC_POSITION GetCurrentPosition(float timeSinceLaunch) const = 0;

		// ==========================================================================
		// 콜백 함수 주입 인터페이스
		// ==========================================================================
	
		// LaunchTubeManager가 발사 완료 알림을 전달
		virtual void WeaponLaunched(std::chrono::steady_clock::time_point launchTime) = 0;
	
		// WpnStatusCtrlManager의 콜백에서 호출됨 (교전계획 준비 상태 확인)
		virtual bool IsEngagementPlanReady() const = 0;

	protected:
		virtual void WeaponSpecInitialization() = 0;
	};



	// =============================================================================
	// 미사일 전용 교전계획 관리자 인터페이스 (ALM, ASM 공통)
	// =============================================================================

	class IMissileEngagementManager : public IEngagementManager {
	public:
		virtual ~IMissileEngagementManager() = default;

		virtual bool SetTargetPosition(const SGEODETIC_POSITION& targetPos) = 0;
		virtual bool SetSystemTarget(uint32_t systemTargetId) = 0;
		virtual std::vector<ST_3D_GEODETIC_POSITION> CalculateTurningPoints() const = 0;

	};
} // namespace MINEASMALM
