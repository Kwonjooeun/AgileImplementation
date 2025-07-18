#include "MineEngagementManager.h"
#include <cstring>

namespace AIEP {

    // =============================================================================
    // MineEngagementManager 구현
    // =============================================================================

    MineEngagementManager::MineEngagementManager(WeaponAssignmentInfo weaponAssignInfo,
        std::shared_ptr<DdsComm> ddsComm)
        : EngagementManagerBase(weaponAssignInfo, ddsComm)
    {
        memset(&m_dropPlan, 0, sizeof(m_dropPlan));

        // 자항기뢰에 부설계획이 할당되지 않는 상황은 없으나 
        // 예외 발생 방지를 위해 1번째 부설계획을 디폴트로 설정
        m_dropPlanListNumber = weaponAssignInfo.dropPlanListNumber.value_or(1);
        m_dropPlanNumber = weaponAssignInfo.dropPlanNumber.value_or(1);

        SetMineDropPlan(m_dropPlanListNumber, m_dropPlanNumber);

        //DroppingPlanManager->readDroppingPlanfromFile(MINE_PLAN_FILE, m_dropPlanListNumber, m_dropPlanNumber, m_dropPlan);
    }

    void MineEngagementManager::UpdateEngagementResult() {
        if (m_shutdown.load()) return;

        try {
            std::lock_guard<std::mutex> dataLock(m_dataMutex);
            std::lock_guard<std::mutex> planLock(m_planMutex);

            if (m_isLaunched) // 발사 후
            {
                PlanTrajectory();
            }
            else // 발사 전
            {
                EstimateCurrentStatus();
            }
           // 자항기뢰는 자함 정보와 부설계획만 있으면 준비 완료
            bool hasOwnShip = (m_ownShipInfo.stShipMovementInfo().dShipLatitude() != 0.0 || m_ownShipInfo.stShipMovementInfo().dShipLongitude() != 0.0);

            if (hasOwnShip && m_dropPlanLoaded && ValidateDropPlan()) {
                m_engagementPlanReady.store(true);
            }

        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(ENGAGEMENT) << "Mine engagement plan calculation error: " << e.what() << std::endl;
        }
    }

    void MineEngagementManager::SendEngagementResult() {
        if (!m_ddsComm || m_shutdown.load()) return;

        try {
            AIEP_M_MINE_EP_RESULT result;

            result.enTubeNum() = static_cast<uint32_t>(m_tubeNumber);

            m_ddsComm->Send(result);
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(ENGAGEMENT) << "Failed to send mine engagement result: " << e.what() << std::endl;
        }
    }

    // 발사 전 교전계획 계산
    void MineEngagementManager::PlanTrajectory()
    {

    }

    // 발사 후 탄의 위치 예측
    void MineEngagementManager::EstimateCurrentStatus()
    {

    }

    bool MineEngagementManager::SetMineDropPlan(uint32_t listNum, uint32_t planNum) {
        std::lock_guard<std::mutex> lock(m_planMutex);

        m_dropPlanListNumber = listNum;
        m_dropPlanNumber = planNum;

        // 부설계획 로드
        try
        {
            DroppingPlanManager->readDroppingPlanfromFile(MINE_PLAN_FILE, m_dropPlanListNumber, m_dropPlanNumber, m_dropPlan);
            m_dropPlanLoaded = true;
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(ENGAGEMENT) << "Mine drop plan setting Fail: " << e.what() << std::endl;
            m_dropPlanLoaded = false;
            return false;
        }
        

        DEBUG_STREAM(ENGAGEMENT) << "Mine drop plan set for Tube " << m_tubeNumber
            << " (List: " << listNum << ", Plan: " << planNum << ")" << std::endl;

        return true;
    }

    bool MineEngagementManager::UpdateDropPlanWaypoints(const std::vector<ST_WEAPON_WAYPOINT>& waypoints) {
        std::lock_guard<std::mutex> lock(m_planMutex);

        if (waypoints.size() > 8) {
            DEBUG_ERROR_STREAM(ENGAGEMENT) << "Too many waypoints for mine drop plan: " << waypoints.size() << std::endl;
            return false;
        }

        // 경로점 업데이트
        m_dropPlan.usWaypointCnt() = static_cast<unsigned short>(waypoints.size());
        for (size_t i = 0; i < waypoints.size(); ++i) {
            m_dropPlan.stWaypoint()[i] = waypoints[i];
        }

        DEBUG_STREAM(ENGAGEMENT) << "Mine drop plan waypoints updated for Tube " << m_tubeNumber
            << ", waypoint count: " << waypoints.size() << std::endl;
        return true;
    }

    bool MineEngagementManager::GetDropPlan(ST_M_MINE_PLAN_INFO& planInfo) const {
        std::lock_guard<std::mutex> lock(m_planMutex);

        if (!m_dropPlanLoaded) {
            return false;
        }

        planInfo = m_dropPlan;
        return true;
    }

    uint32_t MineEngagementManager::GetDropPlanListNumber() const {
        std::lock_guard<std::mutex> lock(m_planMutex);
        return m_dropPlanListNumber;
    }

    uint32_t MineEngagementManager::GetDropPlanNumber() const {
        std::lock_guard<std::mutex> lock(m_planMutex);
        return m_dropPlanNumber;
    }

    bool MineEngagementManager::IsDropPlanLoaded() const {
        std::lock_guard<std::mutex> lock(m_planMutex);
        return m_dropPlanLoaded;
    }

    ST_3D_GEODETIC_POSITION MineEngagementManager::CalculateMinePosition(float timeSinceLaunch) const {
        ST_3D_GEODETIC_POSITION pos;
        memset(&pos, 0, sizeof(pos));

        // 자항기뢰 위치 계산 로직
        // (실제로는 부설계획과 속도를 기반으로 계산)

        return pos;
    }

    bool MineEngagementManager::ValidateDropPlan() const {
        // 부설계획 유효성 검증 로직
        return m_dropPlanLoaded;
    }
} // namespace AIEP
