#include "TubeMessageReceiver.h"
#include <iostream>

namespace MINEASMALM {
    TubeMessageReceiver::TubeMessageReceiver(int tubeNumber, LaunchTubeManager* launchtubemanager, std::shared_ptr<DdsComm> ddsComm)
        : m_tubeNumber(tubeNumber)
        , m_launchtubemanager(launchtubemanager)
        , m_ddsComm(ddsComm)
        , m_initialized(false)
        , m_shutdown(false)
    {}

    TubeMessageReceiver::~TubeMessageReceiver() {
        Shutdown();
    }

    bool TubeMessageReceiver::Initialize() {
        if (m_initialized.load()) {
            return true;
        }

        if (!m_ddsComm || !m_launchtubemanager) {
            DEBUG_ERROR_STREAM(MESSAGERECEIVER) << "Invalid parameters for TubeMessageHandler initialization" << std::endl;
            return false;
        }

        try {

            DEBUG_STREAM(MESSAGERECEIVER) << "Initializing TubeMessageHandler for Tube " << m_tubeNumber << std::endl;

            // DDS Reader 콜백 등록
            m_ddsComm->RegisterReader<TEWA_ASSIGN_CMD>(
                [this](const TEWA_ASSIGN_CMD& msg) { OnAssignCommandReceived(msg); });

            //m_ddsComm->RegisterReader<CMSHCI_AIEP_WPN_CTRL_CMD>(
            //    [this](const CMSHCI_AIEP_WPN_CTRL_CMD& msg) { OnWeaponControlCommandReceived(msg); });

            //m_ddsComm->RegisterReader<CMSHCI_AIEP_WPN_GEO_WAYPOINTS>(
            //    [this](const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& msg) { OnWaypointsReceived(msg); });

            //m_ddsComm->RegisterReader<NAVINF_SHIP_NAVIGATION_INFO>(
            //    [this](const NAVINF_SHIP_NAVIGATION_INFO& msg) { OnOwnShipInfoReceived(msg); });

            //m_ddsComm->RegisterReader<TRKMGR_SYSTEMTARGET_INFO>(
            //    [this](const TRKMGR_SYSTEMTARGET_INFO& msg) { OnTargetInfoReceived(msg); });

            //m_ddsComm->RegisterReader<CMSHCI_AIEP_M_MINE_DROPPING_PLAN_REQ>(
            //    [this](const CMSHCI_AIEP_M_MINE_DROPPING_PLAN_REQ& msg) { OnMineDropPlanRequestReceived(msg); });

            //m_ddsComm->RegisterReader<CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST>(
            //    [this](const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg) { OnEditedPlanListReceived(msg); });

            //m_ddsComm->RegisterReader<CMSHCI_AIEP_M_MINE_SELECTED_PLAN>(
            //    [this](const CMSHCI_AIEP_M_MINE_SELECTED_PLAN& msg) { OnSelectedPlanReceived(msg); });

            m_initialized.store(true);

            DEBUG_STREAM(MESSAGERECEIVER) << "TubeMessageHandler for Tube " << m_tubeNumber << " initialized successfully" << std::endl;

            return true;
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(MESSAGERECEIVER) << "Failed to initialize TubeMessageHandler: " << e.what() << std::endl;
            return false;
        }
    }

    void TubeMessageReceiver::Shutdown() {
        if (!m_initialized.load() || m_shutdown.load()) {
            return;
        }

        DEBUG_STREAM(MESSAGERECEIVER) << "Shutting down TubeMessageHandler for Tube " << m_tubeNumber << std::endl;

        m_shutdown.store(true);
        m_initialized.store(false);

        DEBUG_STREAM(MESSAGERECEIVER) << "TubeMessageHandler for Tube " << m_tubeNumber << " shutdown completed" << std::endl;
    }

    // 유효성 검사 함수들
    bool TubeMessageReceiver::IsMessageForThisTube(int messageTargetTube) const {
        return messageTargetTube == m_tubeNumber;
    }

    // DDS 메시지 수신 콜백 구현
    void TubeMessageReceiver::OnAssignCommandReceived(const TEWA_ASSIGN_CMD& message) {
        int targetTube = static_cast<int>(message.stWpnAssign().enTubeNum());

        if (!IsMessageForThisTube(targetTube)) {
            return;
        }

        DEBUG_STREAM(MESSAGERECEIVER) << "Tube " << m_tubeNumber << " received TEWA_ASSIGN_CMD" << std::endl;

        bool success = false;

        try {
            if (message.eSetCmd() == static_cast<uint32_t>(EN_SET_CMD::SET_CMD_SET)) {
                success = m_launchtubemanager->AssignWeapon(message);
                if (success) {
                    AIEP_ASSIGN_RESP RespMsg;
                    RespMsg.eSetCmd() = static_cast<uint32_t>(EN_SET_CMD::SET_CMD_SET);
                    RespMsg.stWpnAssign() = message.stWpnAssign();

                    m_ddsComm->Send(RespMsg);
                    DEBUG_STREAM(MESSAGERECEIVER) << "Sucessfully Sent: AIEP_ASSIGN_RESP - SET" << std::endl;
                }
                else
                {
                    DEBUG_ERROR_STREAM(MESSAGERECEIVER) << "Failed to assign weapon" << std::endl;
                }
            }
            else if (message.eSetCmd() == static_cast<uint32_t>(EN_SET_CMD::SET_CMD_UNSET)) {
                success = m_launchtubemanager->UnassignWeapon();
                if (success) {
                    AIEP_ASSIGN_RESP RespMsg;
                    RespMsg.eSetCmd() = static_cast<uint32_t>(EN_SET_CMD::SET_CMD_UNSET);
                    RespMsg.stWpnAssign() = message.stWpnAssign();

                    m_ddsComm->Send(RespMsg);
                    DEBUG_STREAM(MESSAGERECEIVER) << "Sucessfully Sent: AIEP_ASSIGN_RESP - UNSET" << std::endl;                    
                }
                else
                {
                    DEBUG_ERROR_STREAM(MESSAGERECEIVER) << "Failed to unassign weapon" << std::endl;
                }
            }
            else {
                DEBUG_ERROR_STREAM(MESSAGERECEIVER) << "Unknown set command" << std::endl;
            }
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(MESSAGERECEIVER) << e.what() << std::endl;
        }
    }
} // namespace MINEASMALM
