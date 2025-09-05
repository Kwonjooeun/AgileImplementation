#include "LaunchTubeManager.h"

//#include "Common/Utils/DebugPrint.h"

namespace AIEP {
    LaunchTubeManager::LaunchTubeManager(int tubeNumber, std::shared_ptr<AIEP::DdsComm> ddsComm)
        : m_tubeNumber(tubeNumber)
        , m_ddsComm(ddsComm)
        , m_initialized(false)
        , m_isAssigned(false)
        , m_weaponKind(static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_NA))
        , m_wpnStatusCtrlManager(nullptr)
        , m_shutdown(false)
    {
        if (!ddsComm) {
            throw std::invalid_argument("DdsComm cannot be null");
        }

        // 할당 정보 초기화
        memset(&m_assignmentInfo, 0, sizeof(m_assignmentInfo));
        memset(&m_ownShipInfo, 0, sizeof(m_ownShipInfo));

        Initialize();
    }

    LaunchTubeManager::~LaunchTubeManager() {
        Shutdown();
    }

    bool LaunchTubeManager::Initialize() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_initialized.load()) {
            return true;
        }

        try {
            DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Initializing LaunchTubeManager " << m_tubeNumber << std::endl;

            // 초기 상태 설정
            m_shutdown.store(false);
            m_initialized.store(true);

            DEBUG_STREAM(LAUNCHTUBEMANAGER) << "LaunchTubeManager# " << m_tubeNumber << " initialized successfully" << std::endl;

            return true;
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Failed to initialize LaunchTubeManager " << m_tubeNumber << ": " << e.what() << std::endl;
            return false;
        }
    }

    void LaunchTubeManager::Shutdown() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_initialized.load() || m_shutdown.load()) {
            return;
        }

        DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Shutting down LaunchTubeManager " << m_tubeNumber << std::endl;

        m_shutdown.store(true);

        // 무장 할당 해제
        if (m_isAssigned) {
            UnassignWeapon();
        }

        // 무장 상태 통제 관리자 종료
        if (m_wpnStatusCtrlManager) {
            m_wpnStatusCtrlManager->Shutdown();
            m_wpnStatusCtrlManager.reset();
        }

        if (m_engagementManager) {
            m_engagementManager->Shutdown();
            m_engagementManager.reset();
        }

        m_initialized.store(false);
        DEBUG_STREAM(LAUNCHTUBEMANAGER) << "LaunchTubeManager " << m_tubeNumber << " shutdown completed" << std::endl;
    }

    void LaunchTubeManager::UpdateLoadedWeaponKind(EN_WPN_KIND weaponKind) {
        m_loadedWeaponKind.store(static_cast<uint32_t>(weaponKind));
    }

    bool LaunchTubeManager::IsWeaponTypeCompatible(const TEWA_ASSIGN_CMD& assignCmd) const {
        uint32_t loadedKind = m_loadedWeaponKind.load();
        uint32_t requestedKind = assignCmd.stWpnAssign().enWeaponType();

        return (loadedKind != static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_NA)) &&
            (loadedKind == requestedKind);
    }

    bool LaunchTubeManager::AssignWeapon(const TEWA_ASSIGN_CMD& assignCmd) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 적재 정보와 일치하는 할당 명령인지 확인
        if (!IsWeaponTypeCompatible(assignCmd))
        {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Cannot assign weapon - weapon type mismatch or no weapon loaded" << std::endl;
            return false;
        }

        try {
            if (!m_isAssigned) // 할당
            {
                m_weaponKind = assignCmd.stWpnAssign().enWeaponType();
                ST_WA_SESSION AssignInfo;
                AssignInfo = assignCmd.stWpnAssign();
                EngagementPlanningFactory& Factory = EngagementPlanningFactory::GetInstance();

                DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Assigning weapon " << static_cast<int>(m_weaponKind)
                    << " to LaunchTube " << m_tubeNumber << std::endl;

                // 1. 무장 상태 통제 관리자 생성
                m_wpnStatusCtrlManager = std::make_unique<WpnStatusCtrlManager>(
                    m_tubeNumber,
                    m_weaponKind,
                    m_ddsComm  // DDS 통신 주입
                    );

                // 2. 교전계획 관리자 생성
                m_engagementManager = Factory.CreateEngagementManager(AssignInfo, m_ddsComm);

                // 3. 콜백 함수들로 두 매니저 연결
                DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Connecting managers with callback functions..." << std::endl;

                // ㄴWpnStatusCtrlManager → LaunchTubeManager → EngagementManager (발사 완료 알림)
                m_wpnStatusCtrlManager->SetLaunchCompletedNotifier([this](std::chrono::steady_clock::time_point launchTime) {
                    OnWeaponLaunched(launchTime);
                    });

                // ㄴWpnStatusCtrlManager가 교전계획 준비 상태를 확인할 수 있도록 체크 함수 주입
                m_wpnStatusCtrlManager->SetEngagementPlanChecker([this]() {
                    return m_engagementManager->IsEngagementPlanReady();
                    });

                // 4. 교전계획 workerloop 시작
                m_engagementManager->StartEngagementPlanManager();

                m_assignmentInfo = assignCmd;
                m_isAssigned = true;
                return true;
            }
            else // 할당 정보 변경
            {
                if (m_engagementManager 
                    && m_wpnStatusCtrlManager->GetCurrentState() != EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH
                    && m_wpnStatusCtrlManager->GetCurrentState() != EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POST_LAUNCH)
                {
                    ST_WA_SESSION AssignInfo;

                    if (m_engagementManager->UpdateWeaponAssignmentInformation(AssignInfo))
                    {
                        m_assignmentInfo = assignCmd;
                        return true;
                    }
                    else // 유효하지 않은 할당 정보 변경
                    {
                        return false;
                    }
                }
                else
                {
                    DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Weapon is assigned but there is no EngagementManager or Weapon was already launched." << std::endl;
                    return false;
                }
            }
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Exception during weapon assignment: " << e.what() << std::endl;
            return false;
        }
    }

    bool LaunchTubeManager::UnassignWeapon() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_isAssigned) {
            return true;
        }
        
        try {
            if ((m_wpnStatusCtrlManager->GetCurrentState() == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF) || m_shutdown.load())
            {
                m_wpnStatusCtrlManager->Shutdown();
                m_engagementManager->Shutdown();

                m_wpnStatusCtrlManager.reset();
                m_engagementManager.reset();

                m_isAssigned = false;
                memset(&m_assignmentInfo, 0, sizeof(m_assignmentInfo));
                m_weaponKind = static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_NA);

                DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Unassigning weapon from LaunchTube " << m_tubeNumber << std::endl;
                return true;
            }
            else
            {
                DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Fail to unassign weapon: weapon status is not off" << std::endl;
                return false;
            }
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Exception during weapon unassignment: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool LaunchTubeManager::IsCommandForThisWeapon(uint32_t weaponKindInCmd)
    {
        return (m_weaponKind == weaponKindInCmd && weaponKindInCmd != static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_NA));
    }

    bool LaunchTubeManager::ProcessPAInfo(const CMSHCI_AIEP_PA_INFO& paInfo) {
        if (!m_isAssigned || !m_engagementManager)
        {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "No engagement plan manager assigned to process PA Info" << std::endl;
            return false;
       }
        m_engagementManager->UpdatePAInfo(paInfo);
    }

    bool LaunchTubeManager::ProcessWeaponControlCommand(const CMSHCI_AIEP_WPN_CTRL_CMD& command) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_isAssigned || !m_wpnStatusCtrlManager) {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "No weapon assigned to process control command" << std::endl;
            return false;
        }

        uint32_t wpnKind{ command.eWpnKind()};
        if (IsCommandForThisWeapon(wpnKind))
        {
            DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Processing weapon control command for Tube " << m_tubeNumber << std::endl;
            return m_wpnStatusCtrlManager->ProcessControlCommand(command);
        }
        else
        {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Invalid Weapon Control Command: Weapon kind mismatch" << std::endl;
            return false;
        }        
    }

    bool LaunchTubeManager::ProcessWaypointCommand(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& command) {
        if (!m_isAssigned || !m_engagementManager)
        {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "No engagement plan manager assigned to process waypoint command" << std::endl;
            return false;
        }

        uint32_t wpnKind{ command.eWpnKind() };
        if (IsCommandForThisWeapon(wpnKind))
        {
            m_engagementManager->UpdateWaypoints(command);
            return true;
        }
        else
        {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Invalid Waypoint Command: Weapon kind mismatch" << std::endl;
            return false;
        }
    }

    bool LaunchTubeManager::ProcessOwnshipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownshipInfo) {
        if (!m_isAssigned || !m_engagementManager)
        {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "No engagement plan manager assigned to process OwnshipInfo" << std::endl;
            return false;
        }
        m_engagementManager->UpdateOwnShipInfo(ownshipInfo);
    }

    bool LaunchTubeManager::ProcessSystemTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& systemtargetInfo) {
        if (!m_isAssigned || !m_engagementManager)
        {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "No engagement plan manager assigned to process SystemTargetInfo" << std::endl;
            return false;
        }
        if ((m_assignmentInfo.stWpnAssign().enAllocTrack() == 1))
        {
            if (m_assignmentInfo.stWpnAssign().unTrackNumber() != 0 
                && m_assignmentInfo.stWpnAssign().unTrackNumber() == systemtargetInfo.unTargetSystemID())
            {
                m_engagementManager->UpdateSystemTargetInfo(systemtargetInfo);
            }            
        }        
    }

    bool LaunchTubeManager::ProcessAIWaypointsInferenceRequest(const CMSHCI_AIEP_AI_WAYPOINTS_INFERENCE_REQ& command)
    {
        if (!m_isAssigned || !m_engagementManager)
        {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "No engagement plan manager assigned to process waypoint command" << std::endl;
            return false;
        }

        uint32_t wpnKind{ command.eWpnKind() };
        if (IsCommandForThisWeapon(wpnKind))
        {
            m_engagementManager->RequestAIWaypointInference(command);
            return true;
        }
        else
        {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Invalid AI Waypoint Inference Request: Weapon kind mismatch" << std::endl;
            return false;
        }
    }

    bool LaunchTubeManager::ProcessAIWaypointsInferenceResult(const AIEP_INTERNAL_INFER_RESULT_WP& command)
    {
        if (!m_isAssigned || !m_engagementManager)
        {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "No engagement plan manager assigned to process AI waypoint result" << std::endl;
            return false;
        }
        m_engagementManager->ProcessAIInferredWaypoints(command);
        return true;
    }


    // ==========================================================================
    // 핵심 콜백 함수들 (매니저 간 상호작용 처리)
    // ==========================================================================
    void LaunchTubeManager::OnWeaponLaunched(std::chrono::steady_clock::time_point launchTime) {
        DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Tube " << m_tubeNumber << " - Weapon launched callback received" << std::endl;

        if (!m_engagementManager) {
            return;
        }

        // EngagementManager에게 발사 완료 알림
        m_engagementManager->WeaponLaunched(launchTime);
        DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Tube " << m_tubeNumber << " notified engagement manager of launch" << std::endl;
    }

} // namespace AIEP
