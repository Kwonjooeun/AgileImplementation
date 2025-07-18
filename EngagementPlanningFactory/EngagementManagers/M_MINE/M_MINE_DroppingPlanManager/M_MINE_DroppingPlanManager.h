#pragma once

#include "../../../dds_message/AIEP_AIEP_.hpp"
#include "../../../Common/Utils/ConfigManager.h"
#include "../EngagementManagerBase.h"
#include "M_MINE_DroppingPlanManager/M_Mine_DroppingPlanManager.h"
#include "M_MINE_Model/M_MINE_Dynamics.h"
#include <memory>
#include <vector>
#include <chrono>

namespace MINEASMALM {
    // =============================================================================
    // 자항기뢰 전용 교전계획 관리자
    // =============================================================================
    class MineEngagementManager : public EngagementManagerBase {
    public:
        MineEngagementManager(WeaponAssignmentInfo weaponAssignInfo, std::shared_ptr<DdsComm> ddsComm);
        ~MineEngagementManager() = default;

        // ==========================================================================
        // 부설계획 관리
        // ==========================================================================
        bool SetMineDropPlan(uint32_t listNum, uint32_t planNum);
        bool UpdateDropPlanWaypoints(const std::vector<ST_WEAPON_WAYPOINT>& waypoints);
        bool GetDropPlan(ST_M_MINE_PLAN_INFO& planInfo) const;

        uint32_t GetDropPlanListNumber() const;
        uint32_t GetDropPlanNumber() const;
        bool IsDropPlanLoaded() const;

    protected:
        // EngagementManagerBase 구현
        void UpdateEngagementResult() override;
        void SendEngagementResult() override;

        void PlanTrajectory() override;
        void EstimateCurrentStatus() override;
    private:

        M_MineDroppingPlanManager* DroppingPlanManager;
        
        // 부설계획 정보
        ST_M_MINE_PLAN_INFO m_dropPlan;
        bool m_dropPlanLoaded{ false };
        uint32_t m_dropPlanListNumber{ 0 };
        uint32_t m_dropPlanNumber{ 0 };
        mutable std::mutex m_planMutex;

        // 자항기뢰 위치 계산
        ST_3D_GEODETIC_POSITION CalculateMinePosition(float timeSinceLaunch) const;

        const std::string MINE_PLAN_FILE = "Hello.json";

        // 부설 계획 검증
        bool ValidateDropPlan() const;
    };

} // namespace AIEP
