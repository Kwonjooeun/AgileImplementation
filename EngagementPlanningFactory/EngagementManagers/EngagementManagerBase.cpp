#include "EngagementManagerBase.h"
#include <cstring>

namespace AIEP {
    // =============================================================================
    // EngagementManagerBase 구현
    // =============================================================================

    EngagementManagerBase::EngagementManagerBase(WeaponAssignmentInfo weaponAssignInfo, std::shared_ptr<DdsComm> ddsComm)
        : m_tubeNumber(weaponAssignInfo.tubeNumber)
        , m_weaponKind(weaponAssignInfo.weaponKind)
        , m_ddsComm(ddsComm)
    {
        // 초기화
        memset(&m_ownShipInfo, 0, sizeof(m_ownShipInfo));
        memset(&m_targetInfo, 0, sizeof(m_targetInfo));
        memset(&m_waypoints, 0, sizeof(m_waypoints));

        m_shutdown.store(false);
        m_initialized.store(true);

        WeaponSpecInitialization();

        DEBUG_STREAM(ENGAGEMENT) << "EngagementManagerBase created for Tube " << m_tubeNumber
            << ", Weapon: " << static_cast<int>(m_weaponKind) << std::endl;
    }

    EngagementManagerBase::~EngagementManagerBase() {
        Shutdown();
    }

    void EngagementManagerBase::WeaponSpecInitialization()
    {
        auto& config = ConfigManager::GetInstance();
        if (!config.LoadFromFile("config.ini")) {
            return;
        }

        m_weaponSpec = config.GetWeaponSpec(m_weaponKind);
    }

    void EngagementManagerBase::Shutdown() {
        if (!m_initialized.load()) {
            return;
        }

        DEBUG_STREAM(ENGAGEMENT) << "EngagementManagerBase shutdown for Tube " << m_tubeNumber << std::endl;

        m_shutdown.store(true);

        m_initialized.store(false);
    }

    void EngagementManagerBase::Reset() {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        m_engagementPlanReady.store(false);
        m_isLaunched.store(false);

        memset(&m_ownShipInfo, 0, sizeof(m_ownShipInfo));
        memset(&m_targetInfo, 0, sizeof(m_targetInfo));
        memset(&m_waypoints, 0, sizeof(m_waypoints));

        DEBUG_STREAM(ENGAGEMENT) << "EngagementManagerBase reset for Tube " << m_tubeNumber << std::endl;
    }

    void EngagementManagerBase::WorkerLoop() {
        auto lastCalculation = std::chrono::steady_clock::now();
        auto lastResultSend = std::chrono::steady_clock::now();

        DEBUG_STREAM(ENGAGEMENT) << "EngagementManager WorkerLoop started for Tube " << m_tubeNumber << std::endl;

        while (!m_shutdown.load()) {
            auto now = std::chrono::steady_clock::now();

            // 1초마다 결과 송신
            if (now - lastResultSend >= std::chrono::seconds(1)) {
                UpdateEngagementResult();
                SendEngagementResult();
                lastResultSend = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        DEBUG_STREAM(ENGAGEMENT) << "EngagementManager WorkerLoop ended for Tube " << m_tubeNumber << std::endl;
    }

    // LaunchTubeManager에서 직접 호출
    void EngagementManagerBase::UpdateOwnShipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownShip) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_ownShipInfo = ownShip;

        DEBUG_STREAM(ENGAGEMENT) << "Tube " << m_tubeNumber << " ownship info updated" << std::endl;

        // 자함 정보가 업데이트되면 교전계획 재계산 (WorkerLoop에서 자동 처리됨)
    }

    // LaunchTubeManager에서 직접 호출
    void EngagementManagerBase::UpdateTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& target) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_targetInfo = target;

        DEBUG_STREAM(ENGAGEMENT) << "Tube " << m_tubeNumber << " target info updated" << std::endl;
    }

    // LaunchTubeManager에서 직접 호출
    void EngagementManagerBase::UpdateWaypoints(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& waypoints) {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_waypoints = waypoints;

        DEBUG_STREAM(ENGAGEMENT) << "Tube " << m_tubeNumber << " waypoints updated" << std::endl;
    }

    void EngagementManagerBase::WeaponLaunched(std::chrono::steady_clock::time_point launchTime) {

        m_isLaunched.store(true);
        m_launchTime = launchTime;

        DEBUG_STREAM(ENGAGEMENT) << "Tube " << m_tubeNumber << " weapon launched - switching to post-launch mode" << std::endl;
    }

    ST_3D_GEODETIC_POSITION EngagementManagerBase::GetCurrentPosition(float timeSinceLaunch) const {
        ST_3D_GEODETIC_POSITION pos;
        memset(&pos, 0, sizeof(pos));

        // 기본 구현 (하위 클래스에서 오버라이드)
        return pos;
    }
} // namespace AIEP
