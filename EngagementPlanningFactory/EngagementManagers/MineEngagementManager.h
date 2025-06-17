#pragma once

#include "../../dds_message/AIEP_AIEP_.hpp"
#include "IEngagementManager.h"
#include <memory>
#include <vector>
#include <chrono>


namespace MINEASMALM {
    
    // =============================================================================
    // 자항기뢰 전용 교전계획 관리자
    // =============================================================================
    class MineEngagementManager : public IEngagementManager {
    public:
        ~MineEngagementManager() = default;

        bool Initialize(int tubeNumber, EN_WPN_KIND weaponKind) override;
        void Reset() override;
        void Shutdown() override;

        void UpdateOwnShipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownShip) override;
        void UpdateTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& target) override;
        void UpdateWaypoints(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& waypoints) override;

        void SetLaunched(bool launched) override;
        bool IsLaunched() const override;

        ST_3D_GEODETIC_POSITION GetCurrentPosition(float timeSinceLaunch) const override;

        // ==========================================================================
        // 부설계획 관리
        // ==========================================================================
        bool SetMineDropPlan(uint32_t listNum, uint32_t planNum);
        bool UpdateDropPlanWaypoints(const std::vector<ST_WEAPON_WAYPOINT>& waypoints);
        bool GetDropPlan(ST_M_MINE_PLAN_INFO& planInfo) const;

        // ==========================================================================
        // 교전계획 결과 오버로딩
        // ==========================================================================
        bool GetEngagementResult(AIEP_M_MINE_EP_RESULT& result);

        uint32_t GetDropPlanListNumber() const;
        uint32_t GetDropPlanNumber() const;
        bool IsDropPlanLoaded() const;
    };
} // namespace MINEASMALM
