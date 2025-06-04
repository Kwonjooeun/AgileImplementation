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

    // 무장 할당 관리
    bool AssignWeapon(const TEWA_ASSIGN_CMD& assignCmd);
    bool UnassignWeapon();
    bool HasWeapon() const { return m_weapon != nullptr; }

    // 무장 상태 통제
    bool RequestStateChange(EN_WPN_CTRL_STATE newState);
    EN_WPN_CTRL_STATE GetCurrentState() const;
    bool IsLaunched() const;

    // 경로점 업데이트
    bool UpdateWaypoints(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& waypoints);

    // 교전계획 관리
    bool CalculateEngagementPlan();
    bool GetEngagementResult(AIEP_M_MINE_EP_RESULT& result) const;        // 자항기뢰용
    bool GetEngagementResult(AIEP_ALM_ASM_EP_RESULT& result) const;       // 미사일용

    // 환경 정보 업데이트
    void UpdateOwnShipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownShip);
    void UpdateTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& target);

    // 상태 조회
    int GetTubeNumber() const { return m_tubeNumber; }
    EN_WPN_KIND GetWeaponKind() const;
    bool IsEngagementPlanValid() const;

    // 마지막 업데이트 시간 조회 (통신 상태 확인용)
    std::chrono::steady_clock::time_point GetLastUpdateTime() const { return m_lastUpdateTime; }

private:
    // 무장별 할당 처리
    bool AssignMineWeapon(const TEWA_ASSIGN_CMD& assignCmd);
    bool AssignMissileWeapon(const TEWA_ASSIGN_CMD& assignCmd);

    // 내부 상태 업데이트
    void UpdateInternalState();
    void CheckInterlockConditions();

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

    // 교전계획 결과
    bool m_engagementPlanValid;
    std::chrono::steady_clock::time_point m_lastEngagementCalcTime;

    // 동기화
    mutable std::mutex m_mutex;
    std::chrono::steady_clock::time_point m_lastUpdateTime;

    // 상태
    std::atomic<bool> m_shutdown;
};
