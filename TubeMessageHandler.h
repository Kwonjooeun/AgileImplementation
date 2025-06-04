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

    // DDS 메시지 송신
    void SendEngagementResult();
    void SendAssignResponse(const TEWA_ASSIGN_CMD& originalCommand, bool success, const std::string& errorMsg = "");
    void SendStatusUpdate();

    // 주기적 작업
    void PeriodicTasks();
    //void CheckConnectionStatus();

    // 메시지 유효성 검사
    bool IsMessageForThisTube(int messageTargetTube) const;
    bool ValidateAssignCommand(const TEWA_ASSIGN_CMD& command) const;
    bool ValidateControlCommand(const CMSHCI_AIEP_WPN_CTRL_CMD& command) const;

    // 멤버 변수
    int m_tubeNumber;
    LaunchTube* m_launchTube;
    std::shared_ptr<DdsComm> m_ddsComm;

    std::atomic<bool> m_initialized;
    std::atomic<bool> m_shutdown;

    // 주기적 작업 타이밍
    std::chrono::steady_clock::time_point m_lastEngagementSendTime;
    std::chrono::steady_clock::time_point m_lastStatusSendTime;
    std::chrono::steady_clock::time_point m_lastConnectionCheckTime;

    // 주기 설정 (ms)
    static const int ENGAGEMENT_SEND_INTERVAL = 1000;  // 1초
    static const int STATUS_SEND_INTERVAL = 1000;      // 1초
    static const int CONNECTION_CHECK_INTERVAL = 10000; // 10초

    // 통계
    uint32_t m_messagesReceived;
    uint32_t m_messagesSent;
    uint32_t m_messagesFiltered;  // 다른 발사관 메시지
};
