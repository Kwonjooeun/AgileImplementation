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
    m_lastUpdateTime = std::chrono::steady_clock::now();
    m_lastEngagementCalcTime = std::chrono::steady_clock::now();

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
        std::cout << "Initializing LaunchTube " << m_tubeNumber << std::endl;

        // 초기 상태 설정
        m_shutdown.store(false);
        m_lastUpdateTime = std::chrono::steady_clock::now();

        m_initialized.store(true);
        std::cout << "LaunchTube " << m_tubeNumber << " initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize LaunchTube " << m_tubeNumber << ": " << e.what() << std::endl;
        return false;
    }
}

void LaunchTube::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized.load() || m_shutdown.load()) {
        return;
    }

    std::cout << "Shutting down LaunchTube " << m_tubeNumber << std::endl;

    m_shutdown.store(true);

    // 무장 할당 해제
    if (m_isAssigned) {
        UnassignWeapon();
    }

    m_initialized.store(false);
    std::cout << "LaunchTube " << m_tubeNumber << " shutdown completed" << std::endl;
}

void LaunchTube::Update() {
    if (!m_initialized.load() || m_shutdown.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    m_lastUpdateTime = std::chrono::steady_clock::now();

    if (m_isAssigned && m_weapon && m_stateManager) {
        try {
            // 무장 상태 업데이트
            m_weapon->Update();

            // 상태 관리자 업데이트
            m_stateManager->Update();

            // 인터록 조건 확인
            CheckInterlockConditions();

            // 주기적 교전계획 계산 (1초마다)
            auto now = std::chrono::steady_clock::now();
            if (now - m_lastEngagementCalcTime > std::chrono::seconds(1)) {
                CalculateEngagementPlan();
                m_lastEngagementCalcTime = now;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error updating LaunchTube " << m_tubeNumber << ": " << e.what() << std::endl;
        }
    }
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

        std::cout << "Assigning weapon " << static_cast<int>(m_weaponKind)
            << " to LaunchTube " << m_tubeNumber << std::endl;

        // WeaponFactory를 통해 무장 관련 객체들 생성
        auto& factory = WeaponFactory::GetInstance();

        m_weapon = factory.CreateWeapon(m_weaponKind);
        if (!m_weapon) {
            std::cerr << "Failed to create weapon" << std::endl;
            return false;
        }

        m_engagementManager = factory.CreateEngagementManager(m_weaponKind);
        if (!m_engagementManager) {
            std::cerr << "Failed to create engagement manager" << std::endl;
            m_weapon.reset();
            return false;
        }

        m_stateManager = factory.CreateWeaponStateManager(m_weaponKind);
        if (!m_stateManager) {
            std::cerr << "Failed to create state manager" << std::endl;
            m_weapon.reset();
            m_engagementManager.reset();
            return false;
        }

        // 초기화
        if (!m_weapon->Initialize(m_tubeNumber)) {
            std::cerr << "Failed to initialize weapon" << std::endl;
            m_weapon.reset();
            m_engagementManager.reset();
            m_stateManager.reset();
            return false;
        }

        if (!m_engagementManager->Initialize(m_tubeNumber, m_weaponKind)) {
            std::cerr << "Failed to initialize engagement manager" << std::endl;
            m_weapon.reset();
            m_engagementManager.reset();
            m_stateManager.reset();
            return false;
        }

        if (!m_stateManager->Initialize(m_tubeNumber, m_weaponKind)) {
            std::cerr << "Failed to initialize state manager" << std::endl;
            m_weapon.reset();
            m_engagementManager.reset();
            m_stateManager.reset();
            return false;
        }

        // 무장별 특수 할당 처리
        bool result = false;
        if (m_weaponKind == EN_WPN_KIND::WPN_KIND_M_MINE) {
            result = AssignMineWeapon(assignCmd);
        }
        else {
            result = AssignMissileWeapon(assignCmd);
        }

        if (result) {
            m_isAssigned = true;
            std::cout << "Weapon successfully assigned to LaunchTube " << m_tubeNumber << std::endl;
        }
        else {
            // 실패시 정리
            m_weapon.reset();
            m_engagementManager.reset();
            m_stateManager.reset();
        }

        return result;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during weapon assignment: " << e.what() << std::endl;
        return false;
    }
}

bool LaunchTube::UnassignWeapon() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isAssigned) {
        return true;
    }

    std::cout << "Unassigning weapon from LaunchTube " << m_tubeNumber << std::endl;

    try {
        // 무장 상태를 OFF로 변경
        if (m_weapon) {
            m_weapon->RequestStateChange(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
        }

        // 객체들 정리
        m_weapon.reset();
        m_engagementManager.reset();
        m_stateManager.reset();

        // 상태 초기화
        m_isAssigned = false;
        m_weaponKind = EN_WPN_KIND::WPN_KIND_NA;
        m_engagementPlanValid = false;
        memset(&m_assignmentInfo, 0, sizeof(m_assignmentInfo));

        std::cout << "Weapon unassigned from LaunchTube " << m_tubeNumber << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during weapon unassignment: " << e.what() << std::endl;
        return false;
    }
}

bool LaunchTube::RequestStateChange(EN_WPN_CTRL_STATE newState) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isAssigned || !m_weapon || !m_stateManager) {
        std::cerr << "No weapon assigned for state change" << std::endl;
        return false;
    }

    std::cout << "LaunchTube " << m_tubeNumber << " requesting state change to "
        << static_cast<int>(newState) << std::endl;

    try {
        return m_stateManager->RequestStateChange(newState);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during state change: " << e.what() << std::endl;
        return false;
    }
}

EN_WPN_CTRL_STATE LaunchTube::GetCurrentState() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isAssigned || !m_weapon) {
        return EN_WPN_CTRL_STATE::WPN_CTRL_STATE_NA;
    }

    return m_weapon->GetCurrentState();
}

bool LaunchTube::IsLaunched() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isAssigned || !m_weapon) {
        return false;
    }

    return m_weapon->IsLaunched();
}

bool LaunchTube::UpdateWaypoints(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& waypoints) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isAssigned || !m_engagementManager) {
        std::cerr << "No weapon assigned for waypoint update" << std::endl;
        return false;
    }

    std::cout << "Updating waypoints for LaunchTube " << m_tubeNumber << std::endl;

    try {
        return m_engagementManager->UpdateWaypoints(waypoints);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during waypoint update: " << e.what() << std::endl;
        return false;
    }
}

bool LaunchTube::CalculateEngagementPlan() {
    if (!m_isAssigned || !m_engagementManager) {
        return false;
    }

    try {
        bool result = m_engagementManager->CalculateEngagementPlan();
        m_engagementPlanValid = result;
        return result;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during engagement plan calculation: " << e.what() << std::endl;
        m_engagementPlanValid = false;
        return false;
    }
}

bool LaunchTube::GetEngagementResult(AIEP_M_MINE_EP_RESULT& result) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isAssigned || !m_engagementManager || m_weaponKind != EN_WPN_KIND::WPN_KIND_M_MINE) {
        return false;
    }

    return m_engagementManager->GetMineEngagementResult(result);
}

bool LaunchTube::GetEngagementResult(AIEP_ALM_ASM_EP_RESULT& result) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_isAssigned || !m_engagementManager ||
        (m_weaponKind != EN_WPN_KIND::WPN_KIND_ALM && m_weaponKind != EN_WPN_KIND::WPN_KIND_ASM)) {
        return false;
    }

    return m_engagementManager->GetMissileEngagementResult(result);
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
bool LaunchTube::AssignMineWeapon(const TEWA_ASSIGN_CMD& assignCmd) {
    // 자항기뢰 특수 할당 처리
    try {
        const auto& wpnAssign = assignCmd.stWpnAssign();

        // 부설계획 정보 설정
        if (m_engagementManager) {
            return m_engagementManager->SetMineDropPlan(
                wpnAssign.usAllocDroppingPlanListNum(),
                wpnAssign.usAllocLayNum()
            );
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in mine weapon assignment: " << e.what() << std::endl;
        return false;
    }
}

bool LaunchTube::AssignMissileWeapon(const TEWA_ASSIGN_CMD& assignCmd) {
    // 미사일 특수 할당 처리
    try {
        const auto& wpnAssign = assignCmd.stWpnAssign();

        // 표적 정보 설정
        if (m_engagementManager) {
            if (wpnAssign.unTrackNumber() > 0) {
                return m_engagementManager->SetSystemTarget(wpnAssign.unTrackNumber());
            }
            else {
                return m_engagementManager->SetTargetPosition(wpnAssign.stTargetPos());
            }
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in missile weapon assignment: " << e.what() << std::endl;
        return false;
    }
}

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
