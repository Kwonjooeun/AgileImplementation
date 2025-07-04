#include "LaunchTubeManager.h"

namespace MINEASMALM {
    LaunchTubeManager::LaunchTubeManager(int tubeNumber)
        : m_tubeNumber(tubeNumber)
        , m_initialized(false)
        , m_isAssigned(false)
        , m_weaponKind(EN_WPN_KIND::WPN_KIND_NA)
        , m_wpnStatusCtrlManager(nullptr)
        , m_shutdown(false)
    {
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
            m_assignmentInfo = assignCmd;
            m_weaponKind = static_cast<EN_WPN_KIND>(assignCmd.stWpnAssign().enWeaponType());

            DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Assigning weapon " << static_cast<int>(m_weaponKind)
                << " to LaunchTube " << m_tubeNumber << std::endl;
            
            // 무장 상태 통제 관리자 생성
            m_wpnStatusCtrlManager = std::make_unique<WpnStatusCtrlManager>();
            if (!m_wpnStatusCtrlManager) {
                DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Failed to create WpnStatusCtrlManager" << std::endl;
                return false;
            }

            // 무장 상태 통제 관리자 초기화
            if (!m_wpnStatusCtrlManager->Initialize(m_tubeNumber, m_weaponKind)) {
                DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Failed to initialize WpnStatusCtrlManager" << std::endl;
                m_wpnStatusCtrlManager.reset();
                return false;
            }
            
            // WeaponFactory를 통해 무장 관련 객체들 생성

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

        //try {
        //     무장 상태 OFF인지 확인 후 OFF 일 경우만 무장 해제 가능

        //    DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Unassigning weapon from LaunchTube " << m_tubeNumber << std::endl;
        //    return true;
        //}
        //catch (const std::exception& e) {
        //    DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Exception during weapon unassignment: " << e.what() << std::endl;
        //    return false;
        //}
    }

    bool LaunchTubeManager::ProcessWeaponControlCommand(const CMSHCI_AIEP_WPN_CTRL_CMD& command) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_isAssigned || !m_wpnStatusCtrlManager) {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "No weapon assigned to process control command" << std::endl;
            return false;
        }

        DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Processing weapon control command for Tube " << m_tubeNumber << std::endl;

        return m_wpnStatusCtrlManager->ProcessControlCommand(command);
    }
} // namespace MINEASMALM
