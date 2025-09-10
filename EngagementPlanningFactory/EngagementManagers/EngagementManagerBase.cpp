#include "EngagementManagerBase.h"
#include "utils/AIEP_DataConverter.h"
#include <cstring>

namespace AIEP {
    // =============================================================================
    // EngagementManagerBase 구현
    // =============================================================================

    EngagementManagerBase::EngagementManagerBase(ST_WA_SESSION weaponAssignInfo, std::shared_ptr<AIEP::DdsComm> ddsComm)
        : m_tubeNumber(weaponAssignInfo.enTubeNum())
        , m_weaponKind(weaponAssignInfo.enWeaponType())
        , m_ddsComm(ddsComm)
        , m_weaponSpec{}
        , m_weaponAssignmentInfo(weaponAssignInfo)
        , m_ownShipInfo{}
        , m_targetInfo{}
        , m_paInfo{}
        , m_waypointCmd{}
    {
        WeaponSpecInitialization();
        m_initialized.store(true);

        DEBUG_STREAM(ENGAGEMENT) << "EngagementManagerBase created for Tube " << m_tubeNumber
            << ", Weapon: " << m_weaponKind << std::endl;
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
        
        if (m_engagementPlanThread.joinable()) {
            m_engagementPlanThread.join();
        }

    }

    void EngagementManagerBase::Reset() {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        m_engagementPlanReady.store(false);
        m_isLaunched.store(false);

        memset(&m_ownShipInfo, 0, sizeof(m_ownShipInfo));
        memset(&m_targetInfo, 0, sizeof(m_targetInfo));
        memset(&m_waypointCmd, 0, sizeof(m_waypointCmd));

        DEBUG_STREAM(ENGAGEMENT) << "EngagementManagerBase reset for Tube " << m_tubeNumber << std::endl;
    }

    void EngagementManagerBase::StartEngagementPlanManager()
    {
        m_engagementPlanThread = std::thread([this]() {WorkerLoop();});
    }

    void EngagementManagerBase::WorkerLoop() {
        auto lastCalculation = std::chrono::steady_clock::now();
        auto lastResultSend = std::chrono::steady_clock::now();

        DEBUG_STREAM(ENGAGEMENT) << "EngagementManager WorkerLoop started for Tube " << m_tubeNumber << std::endl;

        while (!m_shutdown.load()) {
            auto now = std::chrono::steady_clock::now();
            // TODO. timer 개선해야 함
            // 1초마다 결과 송신
            if (now - lastResultSend >= std::chrono::seconds(1)) {
                UpdateEngagementPlanResult();
                SendEngagementPlanResult();
                lastResultSend = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        DEBUG_STREAM(ENGAGEMENT) << "EngagementManager WorkerLoop ended for Tube " << m_tubeNumber << std::endl;
    }

    void EngagementManagerBase::UpdateOwnShipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownShip) {
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_ownShipInfo = ownShip;
        }
        IsInValidLaunchGeometry();
        DEBUG_STREAM(ENGAGEMENT) << "Tube " << m_tubeNumber << " ownship info updated" << std::endl;
    }

    void EngagementManagerBase::UpdateSystemTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& target) {
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_targetInfo = target;
        }
        DEBUG_STREAM(ENGAGEMENT) << "Tube " << m_tubeNumber << " system target info updated" << std::endl;
    }

    void EngagementManagerBase::UpdatePAInfo(const CMSHCI_AIEP_PA_INFO& paInfo) {
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_paInfo = paInfo;
        }
        DEBUG_STREAM(ENGAGEMENT) << "Tube " << m_tubeNumber << " PA info updated" << std::endl;
    }

    void EngagementManagerBase::UpdateWaypoints(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& waypoints) {
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_waypointCmd = waypoints;
        }
        SetWaypoints();

        DEBUG_STREAM(ENGAGEMENT) << "Tube " << m_tubeNumber << " waypoints updated" << std::endl;
    }

    bool EngagementManagerBase::UpdateWeaponAssignmentInformation(const ST_WA_SESSION weaponAssignInfo)
    {
        if (IsValidAssignmentInfo(weaponAssignInfo)) // 유효한 할당 정보인가?
        {
            if (IsAssignmentInfoChanged(weaponAssignInfo)) // 기존 할당 정보와 다른가?
            {
                ApplyWeaponAssignmentInformation(weaponAssignInfo);
                m_weaponAssignmentInfo = weaponAssignInfo;
                return true;
            }
        }

        return false;
    }

    void EngagementManagerBase::RequestAIWaypointInference(const CMSHCI_AIEP_AI_WAYPOINTS_INFERENCE_REQ& AIWPInferReq)
    {
        AIEP_INTERNAL_INFER_REQ msg;
        SetAIWaypointInferenceRequestMessage(msg);
        m_ddsComm->Send(msg);
    }

    void EngagementManagerBase::ProcessAIInferredWaypoints(const AIEP_INTERNAL_INFER_RESULT_WP& AIWPInferReq)
    {
        AIEP_AI_INFER_RESULT_WP msg;        
        ConvertAIWaypointsToGeodetic(AIWPInferReq, msg);
        m_ddsComm->Send(msg);
    }

    void EngagementManagerBase::WeaponLaunched(std::chrono::steady_clock::time_point launchTime) {
        m_isLaunched.store(true);
        m_launchTime = launchTime;
        EngagementPlanInitializationAfterLaunch();
        DEBUG_STREAM(ENGAGEMENT) << "Tube " << m_tubeNumber << " weapon launched - switching to post-launch mode" << std::endl;
    }
} // namespace AIEP
