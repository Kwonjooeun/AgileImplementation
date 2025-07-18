#include "LaunchTubeManager.h"

namespace MINEASMALM {
    LaunchTubeManager::LaunchTubeManager(int tubeNumber, std::shared_ptr<DdsComm> ddsComm)
        : m_tubeNumber(tubeNumber)
        , m_ddsComm(ddsComm)
        , m_initialized(false)
        , m_isAssigned(false)
        , m_weaponKind(EN_WPN_KIND::WPN_KIND_NA)
        , m_wpnStatusCtrlManager(nullptr)
        , m_shutdown(false)
    {
        if (!ddsComm) {
            throw std::invalid_argument("DdsComm cannot be null");
        }
        
        // 할당 정보 초기화
        memset(&m_assignmentInfo, 0, sizeof(m_assignmentInfo));
        memset(&m_ownShipInfo, 0, sizeof(m_ownShipInfo));
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
        
        m_initialized.store(false);
        DEBUG_STREAM(LAUNCHTUBEMANAGER) << "LaunchTubeManager " << m_tubeNumber << " shutdown completed" << std::endl;
    }

    void LaunchTubeManager::UpdateLoadedWeaponKind(EN_WPN_KIND weaponKind) {
        m_loadedWeaponKind.store(static_cast<uint32_t>(weaponKind));
    }

    bool LaunchTubeManager::CanAssignWeapon(const TEWA_ASSIGN_CMD& assignCmd) const {
        uint32_t loadedKind = m_loadedWeaponKind.load();
        uint32_t requestedKind = assignCmd.stWpnAssign().enWeaponType();

        return (loadedKind != static_cast<uint32_t>(EN_WPN_KIND::WPN_KIND_NA)) &&
            (loadedKind == requestedKind);
    }

    bool LaunchTubeManager::AssignWeapon(const TEWA_ASSIGN_CMD& assignCmd) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // 적재 정보와 일치하는 할당 명령인지 확인
        if (!CanAssignWeapon(assignCmd))
        {
            return false;
        }
        
        //// TODO 무장 할당 후에도 할당 정보를 변경할 수 있어야 함. (예를 들어 표적이나 명중지점)
        //if (m_isAssigned) {
        //    DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "LaunchTube " << m_tubeNumber << " already has weapon assigned" << std::endl;
        //    return false;
        //}

        try {
            m_weaponKind = static_cast<EN_WPN_KIND>(assignCmd.stWpnAssign().enWeaponType());
            
            WeaponAssignmentInfo AssignInfo;
            AssignInfo = ConvertFromDdsMessage(assignCmd);
            EngagementPlanningFactory& Factory = EngagementPlanningFactory::GetInstance();
            
            DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Assigning weapon " << static_cast<int>(m_weaponKind)
                << " to LaunchTube " << m_tubeNumber << std::endl;

            // 무장 상태 통제 관리자 생성
            m_wpnStatusCtrlManager = std::make_unique<WpnStatusCtrlManager>(
                m_tubeNumber,
                m_weaponKind,
                m_ddsComm  // DDS 통신 주입
                );

            if (!m_wpnStatusCtrlManager) {
                DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Failed to create WpnStatusCtrlManager" << std::endl;
                return false;
            }

            // 2. 교전계획 관리자 생성
            m_engagementManager = Factory.CreateEngagementManager(AssignInfo, m_ddsComm);

            if (!m_engagementManager )//|| !m_engagementManager->Initialize(m_tubeNumber, m_weaponKind)) 
            {
                DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Failed to create or initialize EngagementManager" << std::endl;
                m_wpnStatusCtrlManager.reset();
                return false;
            }

            // 3. 콜백 함수들로 두 매니저 연결
            DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Connecting managers with callback functions..." << std::endl;

            // WpnStatusCtrlManager → LaunchTubeManager → EngagementManager (발사 완료 알림)
            m_wpnStatusCtrlManager->SetLaunchCompletedNotifier([this](std::chrono::steady_clock::time_point launchTime) {
                OnWeaponLaunched(launchTime);
                });

            // WpnStatusCtrlManager가 교전계획 준비 상태를 확인할 수 있도록 체크 함수 주입
            m_wpnStatusCtrlManager->SetEngagementPlanChecker([this]() {
                return m_engagementManager->IsEngagementPlanReady();
                });

            m_assignmentInfo = assignCmd;
            m_isAssigned = true;
            return true;
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Exception during weapon assignment: " << e.what() << std::endl;
            return false;
        }
    }

    bool LaunchTubeManager::UnassignWeapon() {
        std::lock_guard<std::mutex> lock(m_mutex);
        // TODO. Unassign 로직 추가
        if (!m_isAssigned) {
            return true;
        }

        try {
            if (m_wpnStatusCtrlManager->GetCurrentState() != EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF)
            {
                DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Fail to unassign weapon: weapon status is not off" << std::endl;
                return false;
            }

            m_wpnStatusCtrlManager->Shutdown();
            m_engagementManager->Shutdown();

            m_isAssigned = false;
            memset(&m_assignmentInfo, 0, sizeof(m_assignmentInfo));
            m_weaponKind = EN_WPN_KIND::WPN_KIND_NA;

            DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Unassigning weapon from LaunchTube " << m_tubeNumber << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Exception during weapon unassignment: " << e.what() << std::endl;
            return false;
        }
        return false;
    }

    // 변환 함수 (한 곳에서만 DDS 메시지 구조 알면 됨)
    WeaponAssignmentInfo LaunchTubeManager::ConvertFromDdsMessage(const TEWA_ASSIGN_CMD& ddsMsg) {
        WeaponAssignmentInfo info;

        info.tubeNumber = ddsMsg.stWpnAssign().enTubeNum();
        info.weaponKind = static_cast<EN_WPN_KIND>(ddsMsg.stWpnAssign().enWeaponType());        

        // 무장별 특화 정보 추출
        if (ddsMsg.stWpnAssign().enAllocLay() == 1 || ddsMsg.stWpnAssign().enAllocLay() == 2) {
            info.dropPlanListNumber = ddsMsg.stWpnAssign().usAllocDroppingPlanListNum();
            info.dropPlanNumber = ddsMsg.stWpnAssign().usAllocLayNum();
        }
        else if (ddsMsg.stWpnAssign().enAllocTrack()==1 || ddsMsg.stWpnAssign().enAllocTrack() == 2) {
            info.systemTargetId = ddsMsg.stWpnAssign().unTrackNumber();
        }
        else if (ddsMsg.stWpnAssign().enAllocTarget() == 1 || ddsMsg.stWpnAssign().enAllocTarget() == 2) {
            info.targetPosition = ddsMsg.stWpnAssign().stTargetPos();
        }

        info.requestConsole = ddsMsg.stWpnAssign().enConsoleNum();
        info.allocConsole = ddsMsg.stWpnAssign().enAllocConsoleNum();

        return info;
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
} // namespace MINEASMALM
