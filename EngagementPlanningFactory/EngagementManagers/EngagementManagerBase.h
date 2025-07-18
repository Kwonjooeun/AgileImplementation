#pragma once

#include "IEngagementManager.h"
#include "../../Common/Utils/ConfigManager.h"
#include "../../Common/Utils/DebugPrint.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

namespace MINEASMALM {

    // =============================================================================
    // 기본 교전계획 관리자 (공통 기능)
    // =============================================================================
    class EngagementManagerBase : public IEngagementManager {
    public:
        EngagementManagerBase(WeaponAssignmentInfo weaponAssignInfo, std::shared_ptr<DdsComm> ddsComm);
        virtual ~EngagementManagerBase();

        //bool Initialize(int tubeNumber, EN_WPN_KIND weaponKind) override;
        void Reset() override;
        void Shutdown() override;

        void WorkerLoop() override;

        // DDS 메시지 처리 (LaunchTubeManager에서 직접 호출)
        void UpdateOwnShipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownShip) override;
        void UpdateTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& target) override;
        void UpdateWaypoints(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& waypoints) override;

        ST_3D_GEODETIC_POSITION GetCurrentPosition(float timeSinceLaunch) const override;

        // 발사 완료 여부 확인
        void WeaponLaunched(std::chrono::steady_clock::time_point launchTime) override;

        // 교전계획 준비 상태 확인
        bool IsEngagementPlanReady() const override {
            return m_engagementPlanReady.load();
        }

    protected:
        void WeaponSpecInitialization() override;
        virtual void UpdateEngagementResult() = 0;
        virtual void SendEngagementResult() = 0;

        virtual void PlanTrajectory() = 0;
        virtual void EstimateCurrentStatus() = 0;

        // 멤버 변수
        std::shared_ptr<DdsComm> m_ddsComm;

        std::atomic<bool> m_initialized{ false };
        std::atomic<bool> m_isLaunched{ false };
        std::atomic<bool> m_engagementPlanReady{ false };

        int m_tubeNumber;
        EN_WPN_KIND m_weaponKind;
        std::atomic<bool> m_shutdown{ false };
        WeaponSpecification m_weaponSpec;

        // 발사 시간
        std::chrono::steady_clock::time_point m_launchTime;

        // 환경 정보
        NAVINF_SHIP_NAVIGATION_INFO m_ownShipInfo;
        TRKMGR_SYSTEMTARGET_INFO m_targetInfo;
        CMSHCI_AIEP_WPN_GEO_WAYPOINTS m_waypoints;
        std::mutex m_dataMutex;
    };
} // namespace AIEP
