#pragma once

#include "../../dds_message/AIEP_AIEP_.hpp"
#include <memory>
#include <vector>
#include <chrono>


namespace MINEASMALM {
	// =============================================================================
	// 교전계획 관리자 인터페이스 계층
	// =============================================================================

	class IEngagementManager {
	public:
		virtual ~IEngagementManager() = default;
		virtual bool Initialize(int tubeNumber, EN_WPN_KIND weaponKind) = 0;
		virtual void Reset() = 0;
		virtual void Shutdown() = 0;

		virtual void UpdateOwnShipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownShip) = 0;
		virtual void UpdateTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& target) = 0;
		virtual void UpdateWaypoints(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& waypoints) = 0;

		virtual void SetLaunched(bool launched) = 0;
		virtual bool IsLaunched() const = 0;

		virtual ST_3D_GEODETIC_POSITION GetCurrentPosition(float timeSinceLaunch) const = 0;
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
