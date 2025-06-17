#include "LaunchTubeManager.h"

namespace MINEASMALM {
    LaunchTubeManager::LaunchTubeManager(int tubeNumber)
        : m_tubeNumber(tubeNumber)
        , m_initialized(false)
        , m_isAssigned(false)
        , m_weaponKind(EN_WPN_KIND::WPN_KIND_NA)
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

        m_initialized.store(false);
        DEBUG_STREAM(LAUNCHTUBEMANAGER) << "LaunchTubeManager " << m_tubeNumber << " shutdown completed" << std::endl;
    }

    bool LaunchTubeManager::AssignWeapon(const TEWA_ASSIGN_CMD& assignCmd) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_isAssigned) {
            DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "LaunchTube " << m_tubeNumber << " already has weapon assigned" << std::endl;
            return false;
        }

        try {
            m_assignmentInfo = assignCmd;
            m_weaponKind = static_cast<EN_WPN_KIND>(assignCmd.stWpnAssign().enWeaponType());

            DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Assigning weapon " << static_cast<int>(m_weaponKind)
                << " to LaunchTube " << m_tubeNumber << std::endl;

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

        if (!m_isAssigned) {
            return true;
        }

        //try {
        //     무장 상태를 OFF로 변경 (WeaponStatusCtrlManager 사용)

        //    DEBUG_STREAM(LAUNCHTUBEMANAGER) << "Unassigning weapon from LaunchTube " << m_tubeNumber << std::endl;
        //    return true;
        //}
        //catch (const std::exception& e) {
        //    DEBUG_ERROR_STREAM(LAUNCHTUBEMANAGER) << "Exception during weapon unassignment: " << e.what() << std::endl;
        //    return false;
        //}
    }
    
} // namespace MINEASMALM
