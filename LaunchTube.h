#pragma once

#include "Factory/Weapons/IWeapon.h"
#include "Factory/EngagementManagers/IEngagementManager.h"
#include "Factory/StateManagers/IWeaponStateManager.h"
#include "Common/Types/CommonTypes.h"
#include "dds_message/AIEP_AIEP_.hpp"
#include <memory>
#include <mutex>
#include <atomic>

class LaunchTube {
public:
    explicit LaunchTube(int tubeNumber);
    ~LaunchTube();

    // 초기화 및 종료
    bool Initialize();
    void Shutdown();
    void Update();

private:
    // 멤버 변수
    int m_tubeNumber;
    std::atomic<bool> m_initialized;

    // 무장 관련 객체들
    std::unique_ptr<IWeapon> m_weapon;
    std::unique_ptr<IEngagementManager> m_engagementManager;
    std::unique_ptr<IWeaponStateManager> m_stateManager;

    // 할당 정보
    TEWA_ASSIGN_CMD m_assignmentInfo;
    bool m_isAssigned;
    EN_WPN_KIND m_weaponKind;

    // 환경 정보
    NAVINF_SHIP_NAVIGATION_INFO m_ownShipInfo;
    std::map<uint32_t, TRKMGR_SYSTEMTARGET_INFO> m_targetInfoMap;
};
