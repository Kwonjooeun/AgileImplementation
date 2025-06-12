#include "LaunchTube.h"
#include "Factory/WeaponFactory.h"
#include <iostream>
#include <chrono>

LaunchTube::LaunchTube(int tubeNumber)
    : m_tubeNumber(tubeNumber)
    , m_initialized(false)
    , m_isAssigned(false)
    , m_weaponKind(EN_WPN_KIND::WPN_KIND_NA)
    , m_engagementPlanValid(false)
    , m_shutdown(false)
{
    // 할당 정보 초기화
    memset(&m_assignmentInfo, 0, sizeof(m_assignmentInfo));
    memset(&m_ownShipInfo, 0, sizeof(m_ownShipInfo));
}

LaunchTube::~LaunchTube() {
    Shutdown();
}

bool LaunchTube::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized.load()) {
        return true;
    }

    try {
        // 초기 상태 설정
        m_shutdown.store(false);
        m_initialized.store(true);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

void LaunchTube::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized.load() || m_shutdown.load()) {
        return;
    }

    m_shutdown.store(true);

    // 무장 할당 해제
    if (m_isAssigned) {
        UnassignWeapon();
    }

    m_initialized.store(false);
}

bool LaunchTube::AssignWeapon(const TEWA_ASSIGN_CMD& assignCmd) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isAssigned) {
        std::cerr << "LaunchTube " << m_tubeNumber << " already has weapon assigned" << std::endl;
        return false;
    }

    try {
        m_assignmentInfo = assignCmd;
        m_weaponKind = static_cast<EN_WPN_KIND>(assignCmd.stWpnAssign().enWeaponType());

        // WeaponFactory를 통해 무장 관련 객체들 생성
        auto& factory = WeaponFactory::GetInstance();
        m_isAssigned = true;

        return m_isAssigned;
    }
    catch (const std::exception& e) {
        return m_isAssigned;
    }
}

bool LaunchTube::UnassignWeapon() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isAssigned) {
        return true;
    }
    try {
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

void LaunchTube::UpdateOwnShipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownShip) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_ownShipInfo = ownShip;

    if (m_engagementManager) {
        m_engagementManager->UpdateOwnShipInfo(ownShip);
    }
}

void LaunchTube::UpdateTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& target) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_targetInfoMap[target.unTargetSystemID()] = target;

    if (m_engagementManager) {
        m_engagementManager->UpdateTargetInfo(target);
    }
}

EN_WPN_KIND LaunchTube::GetWeaponKind() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_weaponKind;
}

bool LaunchTube::IsEngagementPlanValid() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_engagementPlanValid;
}

// Private 메서드들
void LaunchTube::CheckInterlockConditions() {
    if (!m_weapon || !m_stateManager) {
        return;
    }

    try {
        // 인터록 조건 확인
        bool interlockOk = true;

        // 교전계획 유효성 확인
        if (!m_engagementPlanValid) {
            interlockOk = false;
        }

        // 무장별 추가 인터록 조건 확인
        // (여기서는 기본적인 조건만 확인)

        // 상태 관리자에 인터록 상태 전달
        m_stateManager->SetInterlockStatus(interlockOk);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in interlock check: " << e.what() << std::endl;
    }
}
