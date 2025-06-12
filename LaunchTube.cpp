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
}
