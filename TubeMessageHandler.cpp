#include "TubeMessageHandler.h"
#include <iostream>
#include <chrono>

TubeMessageHandler::TubeMessageHandler(int tubeNumber, LaunchTube* launchTube, std::shared_ptr<DdsComm> ddsComm)
    : m_tubeNumber(tubeNumber)
    , m_launchTube(launchTube)
    , m_ddsComm(ddsComm)
    , m_initialized(false)
    , m_shutdown(false)
    , m_messagesReceived(0)
    , m_messagesSent(0)
    , m_messagesFiltered(0)
{
    auto now = std::chrono::steady_clock::now();
    m_lastEngagementSendTime = now;
    m_lastStatusSendTime = now;
    m_lastConnectionCheckTime = now;
}

TubeMessageHandler::~TubeMessageHandler() {
    Shutdown();
}

bool TubeMessageHandler::Initialize() {
    if (m_initialized.load()) {
        return true;
    }

    if (!m_ddsComm || !m_launchTube) {
        std::cerr << "Invalid parameters for TubeMessageHandler initialization" << std::endl;
        return false;
    }

    try {
        std::cout << "Initializing TubeMessageHandler for Tube " << m_tubeNumber << std::endl;

        // DDS Reader 콜백 등록
        m_ddsComm->RegisterReader<TEWA_ASSIGN_CMD>(
            [this](const TEWA_ASSIGN_CMD& msg) { OnAssignCommandReceived(msg); });

        m_ddsComm->RegisterReader<CMSHCI_AIEP_WPN_CTRL_CMD>(
            [this](const CMSHCI_AIEP_WPN_CTRL_CMD& msg) { OnWeaponControlCommandReceived(msg); });

        m_ddsComm->RegisterReader<CMSHCI_AIEP_WPN_GEO_WAYPOINTS>(
            [this](const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& msg) { OnWaypointsReceived(msg); });

        m_ddsComm->RegisterReader<NAVINF_SHIP_NAVIGATION_INFO>(
            [this](const NAVINF_SHIP_NAVIGATION_INFO& msg) { OnOwnShipInfoReceived(msg); });


        m_ddsComm->RegisterReader<TRKMGR_SYSTEMTARGET_INFO>(
            [this](const TRKMGR_SYSTEMTARGET_INFO& msg) { OnTargetInfoReceived(msg); });

        m_ddsComm->RegisterReader<CMSHCI_AIEP_M_MINE_DROPPING_PLAN_REQ>(
            [this](const CMSHCI_AIEP_M_MINE_DROPPING_PLAN_REQ& msg) { OnMineDropPlanRequestReceived(msg); });

        m_ddsComm->RegisterReader<CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST>(
            [this](const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg) { OnEditedPlanListReceived(msg); });

        m_ddsComm->RegisterReader<CMSHCI_AIEP_M_MINE_SELECTED_PLAN>(
            [this](const CMSHCI_AIEP_M_MINE_SELECTED_PLAN& msg) { OnSelectedPlanReceived(msg); });

        m_initialized.store(true);
        std::cout << "TubeMessageHandler for Tube " << m_tubeNumber << " initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize TubeMessageHandler: " << e.what() << std::endl;
        return false;
    }
}

void TubeMessageHandler::Shutdown() {
    if (!m_initialized.load() || m_shutdown.load()) {
        return;
    }

    std::cout << "Shutting down TubeMessageHandler for Tube " << m_tubeNumber << std::endl;

    m_shutdown.store(true);

    // 통계 출력
    std::cout << "TubeMessageHandler Statistics:" << std::endl;
    std::cout << "  Messages Received: " << m_messagesReceived << std::endl;
    std::cout << "  Messages Sent: " << m_messagesSent << std::endl;
    std::cout << "  Messages Filtered: " << m_messagesFiltered << std::endl;

    m_initialized.store(false);
    std::cout << "TubeMessageHandler for Tube " << m_tubeNumber << " shutdown completed" << std::endl;
}

void TubeMessageHandler::Update() {
    if (!m_initialized.load() || m_shutdown.load()) {
        return;
    }

    try {
        PeriodicTasks();
        //CheckConnectionStatus();
    }
    catch (const std::exception& e) {
        std::cerr << "Error in TubeMessageHandler update: " << e.what() << std::endl;
    }
}

// DDS 메시지 수신 콜백 구현
void TubeMessageHandler::OnAssignCommandReceived(const TEWA_ASSIGN_CMD& message) {
    m_messagesReceived++;

    int targetTube = static_cast<int>(message.stWpnAssign().enTubeNum());
    if (!IsMessageForThisTube(targetTube)) {
        m_messagesFiltered++;
        return;
    }

    std::cout << "Tube " << m_tubeNumber << " received assign command" << std::endl;

    if (!ValidateAssignCommand(message)) {
        std::cerr << "Invalid assign command for Tube " << m_tubeNumber << std::endl;
        SendAssignResponse(message, false, "Invalid command parameters");
        return;
    }


    bool success = false;
    std::string errorMsg;

    try {
        if (message.eSetCmd() == static_cast<uint32_t>(EN_SET_CMD::SET_CMD_SET)) {
            success = m_launchTube->AssignWeapon(message);
            if (!success) {
                errorMsg = "Failed to assign weapon";
            }
        }
        else if (message.eSetCmd() == static_cast<uint32_t>(EN_SET_CMD::SET_CMD_UNSET)) {
            success = m_launchTube->UnassignWeapon();
            if (!success) {
                errorMsg = "Failed to unassign weapon";
            }
        }
        else {
            errorMsg = "Unknown set command";
        }
    }
    catch (const std::exception& e) {
        errorMsg = e.what();
    }

    SendAssignResponse(message, success, errorMsg);
}

void TubeMessageHandler::OnWeaponControlCommandReceived(const CMSHCI_AIEP_WPN_CTRL_CMD& message) {
    m_messagesReceived++;

    int targetTube = static_cast<int>(message.eTubeNum());
    if (!IsMessageForThisTube(targetTube)) {
        m_messagesFiltered++;
        return;
    }

    std::cout << "Tube " << m_tubeNumber << " received weapon control command" << std::endl;

    if (!ValidateControlCommand(message)) {
        std::cerr << "Invalid control command for Tube " << m_tubeNumber << std::endl;
        return;
    }

    try {
        EN_WPN_CTRL_STATE newState = static_cast<EN_WPN_CTRL_STATE>(message.eWpnCtrlCmd());
        bool success = m_launchTube->RequestStateChange(newState);

        if (success) {
            std::cout << "Tube " << m_tubeNumber << " state change successful" << std::endl;
        }
        else {
            std::cerr << "Tube " << m_tubeNumber << " state change failed" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in weapon control: " << e.what() << std::endl;
    }
}

void TubeMessageHandler::OnWaypointsReceived(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& message) {
    m_messagesReceived++;

    int targetTube = static_cast<int>(message.eTubeNum());
    if (!IsMessageForThisTube(targetTube)) {
        m_messagesFiltered++;
        return;
    }

    std::cout << "Tube " << m_tubeNumber << " received waypoints update" << std::endl;

    try {
        bool success = m_launchTube->UpdateWaypoints(message);

        if (success) {
            std::cout << "Tube " << m_tubeNumber << " waypoints updated successfully" << std::endl;
        }
        else {
            std::cerr << "Tube " << m_tubeNumber << " waypoints update failed" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in waypoints update: " << e.what() << std::endl;
    }
}

void TubeMessageHandler::OnOwnShipInfoReceived(const NAVINF_SHIP_NAVIGATION_INFO& message) {
    m_messagesReceived++;

    // 자함 정보는 모든 발사관에 적용
    try {
        m_launchTube->UpdateOwnShipInfo(message);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in own ship info update: " << e.what() << std::endl;
    }
}

void TubeMessageHandler::OnTargetInfoReceived(const TRKMGR_SYSTEMTARGET_INFO& message) {
    m_messagesReceived++;

    // 표적 정보는 모든 발사관에 적용
    try {
        m_launchTube->UpdateTargetInfo(message);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in target info update: " << e.what() << std::endl;
    }
}

void TubeMessageHandler::OnMineDropPlanRequestReceived(const CMSHCI_AIEP_M_MINE_DROPPING_PLAN_REQ& message) {
    m_messagesReceived++;

    // 부설계획 요청은 자항기뢰가 할당된 발사관에서만 처리
    if (m_launchTube->GetWeaponKind() != EN_WPN_KIND::WPN_KIND_M_MINE) {
        return;
    }

    std::cout << "Tube " << m_tubeNumber << " received mine drop plan request" << std::endl;

    // 현재는 단순히 로그만 출력 (실제로는 계획 관리자와 통신)
}

void TubeMessageHandler::OnEditedPlanListReceived(const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& message) {
    m_messagesReceived++;

    // 편집된 계획 목록은 자항기뢰가 할당된 발사관에서만 처리
    if (m_launchTube->GetWeaponKind() != EN_WPN_KIND::WPN_KIND_M_MINE) {
        return;
    }

    std::cout << "Tube " << m_tubeNumber << " received edited plan list" << std::endl;
}

void TubeMessageHandler::OnSelectedPlanReceived(const CMSHCI_AIEP_M_MINE_SELECTED_PLAN& message) {
    m_messagesReceived++;

    // 선택된 계획은 자항기뢰가 할당된 발사관에서만 처리
    if (m_launchTube->GetWeaponKind() != EN_WPN_KIND::WPN_KIND_M_MINE) {
        return;
    }

    std::cout << "Tube " << m_tubeNumber << " received selected plan" << std::endl;
}

// DDS 메시지 송신 구현
void TubeMessageHandler::SendEngagementResult() {
    if (!m_launchTube->HasWeapon() || !m_launchTube->IsEngagementPlanValid()) {
        return;
    }

    try {
        EN_WPN_KIND weaponKind = m_launchTube->GetWeaponKind();

        if (weaponKind == EN_WPN_KIND::WPN_KIND_M_MINE) {
            AIEP_M_MINE_EP_RESULT result;
            if (m_launchTube->GetEngagementResult(result)) {
                result.enTubeNum() = m_tubeNumber;
                m_ddsComm->Send(result);
                m_messagesSent++;
            }
        }
        else if (weaponKind == EN_WPN_KIND::WPN_KIND_ALM || weaponKind == EN_WPN_KIND::WPN_KIND_ASM) {
            AIEP_ALM_ASM_EP_RESULT result;
            if (m_launchTube->GetEngagementResult(result)) {
                result.enTubeNum() = m_tubeNumber;
                m_ddsComm->Send(result);
                m_messagesSent++;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in sending engagement result: " << e.what() << std::endl;
    }
}

void TubeMessageHandler::SendAssignResponse(const TEWA_ASSIGN_CMD& originalCommand, bool success, const std::string& errorMsg) {
    try {
        AIEP_ASSIGN_RESP response;
        uint32_t SetorUnset{ 0 };

        if (success)
        {
            SetorUnset = 1; // EN_SET_CMD::SET_CMD_SET
        }
        else
        {
            SetorUnset = 2; // EN_SET_CMD::SET_CMD_UNSET
        }
        response.stWpnAssign() = originalCommand.stWpnAssign();
        response.eSetCmd() = SetorUnset;

        m_ddsComm->Send(response);

        m_messagesSent++;

        std::cout << "Sent assign response for Tube " << m_tubeNumber
            << " (success: " << success << ")" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in sending assign response: " << e.what() << std::endl;
    }
}

void TubeMessageHandler::SendStatusUpdate() {
    try {
        // 상태 업데이트 메시지 구성 및 송신 (필요시 구현)
        std::cout << "Tube " << m_tubeNumber << " status update sent" << std::endl;

        AIEP_WPN_CTRL_STATUS_INFO response;

        response.eTubeNum() = m_tubeNumber;
        response.eCtrlState() = static_cast<uint32_t>(m_launchTube->GetCurrentState());
        // TODO. 무장 켠 후 경과시간은 어떻게 저장, 관리할지?
        // response.wpnTime() = ?

        m_ddsComm->Send(response);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in sending status update: " << e.what() << std::endl;
    }
}

// 주기적 작업
void TubeMessageHandler::PeriodicTasks() {
    auto now = std::chrono::steady_clock::now();

    // 교전계획 결과 송신 (1초마다)
    if (now - m_lastEngagementSendTime > std::chrono::milliseconds(ENGAGEMENT_SEND_INTERVAL)) {
        SendEngagementResult();
        m_lastEngagementSendTime = now;
    }

    // 상태 업데이트 송신 (1초마다)
    if (now - m_lastStatusSendTime > std::chrono::milliseconds(STATUS_SEND_INTERVAL)) {
        SendStatusUpdate();
        m_lastStatusSendTime = now;
    }
}

// 유효성 검사 함수들
bool TubeMessageHandler::IsMessageForThisTube(int messageTargetTube) const {
    return messageTargetTube == m_tubeNumber;
}

bool TubeMessageHandler::ValidateAssignCommand(const TEWA_ASSIGN_CMD& command) const {
    // 기본적인 유효성 검사
    const auto& wpnAssign = command.stWpnAssign();

    // 발사관 번호 확인
    if (static_cast<int>(wpnAssign.enTubeNum()) != m_tubeNumber) {
        return false;
    }

    // 무장 종류 확인
    EN_WPN_KIND weaponKind = static_cast<EN_WPN_KIND>(wpnAssign.enWeaponType());
    if (weaponKind == EN_WPN_KIND::WPN_KIND_NA) {
        return false;
    }

    return true;
}

bool TubeMessageHandler::ValidateControlCommand(const CMSHCI_AIEP_WPN_CTRL_CMD& command) const {
    // 발사관 번호 확인
    if (static_cast<int>(command.eTubeNum()) != m_tubeNumber) {
        return false;
    }

    // 상태 확인
    EN_WPN_CTRL_STATE state = static_cast<EN_WPN_CTRL_STATE>(command.eWpnCtrlCmd());
    if (state == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_NA) {
        return false;
    }

    return true;
}
