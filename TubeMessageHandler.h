#pragma once

#include "LaunchTube.h"
#include "Common/Communication/DdsComm.h"
#include "dds_message/AIEP_AIEP_.hpp"
#include <memory>
#include <atomic>
#include <chrono>

class TubeMessageHandler {
public:
    TubeMessageHandler(int tubeNumber, LaunchTube* launchTube, std::shared_ptr<DdsComm> ddsComm);
    ~TubeMessageHandler();

    // 초기화 및 종료
    bool Initialize();
    void Shutdown();
    void Update();

private:
    // DDS 메시지 수신 콜백들
    void OnAssignCommandReceived(const TEWA_ASSIGN_CMD& message);
    void OnWeaponControlCommandReceived(const CMSHCI_AIEP_WPN_CTRL_CMD& message);
    void OnWaypointsReceived(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& message);
    void OnOwnShipInfoReceived(const NAVINF_SHIP_NAVIGATION_INFO& message);
    void OnTargetInfoReceived(const TRKMGR_SYSTEMTARGET_INFO& message);
    void OnMineDropPlanRequestReceived(const CMSHCI_AIEP_M_MINE_DROPPING_PLAN_REQ& message);
    void OnEditedPlanListReceived(const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& message);
    void OnSelectedPlanReceived(const CMSHCI_AIEP_M_MINE_SELECTED_PLAN& message);

    // 멤버 변수
    int m_tubeNumber;
    LaunchTube* m_launchTube;
    std::shared_ptr<DdsComm> m_ddsComm;

    std::atomic<bool> m_initialized;
    std::atomic<bool> m_shutdown;
};
