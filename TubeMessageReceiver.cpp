#include "TubeMessageReceiver.h"
#include <iostream>

namespace AIEP {
    TubeMessageReceiver::TubeMessageReceiver(int tubeNumber, LaunchTubeManager* launchtubemanager, std::shared_ptr<AIEP::DdsComm> ddsComm)
        : m_tubeNumber(tubeNumber)
        , m_launchtubemanager(launchtubemanager)
        , m_ddsComm(ddsComm)
        , m_initialized(false)
        , m_shutdown(false)
    {
        Initialize();
    }

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

            // 적재정보 콜백 등록
            m_ddsComm->RegisterReader<TEWA_WA_TUBE_LOAD_INFO>(
                [this](const TEWA_WA_TUBE_LOAD_INFO& msg) { OnLoadInfoReceived(msg); });

            // DDS Reader 콜백 등록
            m_ddsComm->RegisterReader<TEWA_ASSIGN_CMD>(
                [this](const TEWA_ASSIGN_CMD& msg) { OnAssignCommandReceived(msg); });

            m_ddsComm->RegisterReader<CMSHCI_AIEP_PA_INFO>(
                [this](const CMSHCI_AIEP_PA_INFO& msg) { OnPAInfoReceived(msg); });

            m_ddsComm->RegisterReader<CMSHCI_AIEP_WPN_CTRL_CMD>(
                [this](const CMSHCI_AIEP_WPN_CTRL_CMD& msg) { OnWeaponControlCommandReceived(msg); });

            m_ddsComm->RegisterReader<CMSHCI_AIEP_WPN_GEO_WAYPOINTS>(
                [this](const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& msg) { OnWaypointsReceived(msg); });

            m_ddsComm->RegisterReader<NAVINF_SHIP_NAVIGATION_INFO>(
                [this](const NAVINF_SHIP_NAVIGATION_INFO& msg) { OnOwnShipInfoReceived(msg); });

            m_ddsComm->RegisterReader<CMSHCI_AIEP_AI_WAYPOINTS_INFERENCE_REQ>(
                [this](const CMSHCI_AIEP_AI_WAYPOINTS_INFERENCE_REQ& msg) { OnAIWaypointsInferenceRequestReceived(msg); });

            m_ddsComm->RegisterReader<AIEP_INTERNAL_INFER_RESULT_WP>(
                [this](const AIEP_INTERNAL_INFER_RESULT_WP& msg) { OnAIWaypointsInferenceResultReceived(msg); });

           m_ddsComm->RegisterReader<TRKMGR_SYSTEMTARGET_INFO>(
                [this](const TRKMGR_SYSTEMTARGET_INFO& msg) { OnSystemTargetInfoReceived(msg); });

            //m_ddsComm->RegisterReader<CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST>(
            //    [this](const CMSHCI_AIEP_M_MINE_EDITED_PLAN_LIST& msg) { OnEditedPlanListReceived(msg); });

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

    bool TubeMessageReceiver::IsMessageForThisTube(int messageTargetTube) const {
        return messageTargetTube == m_tubeNumber;
    }

    void TubeMessageReceiver::OnLoadInfoReceived(const TEWA_WA_TUBE_LOAD_INFO& message)
    {
        int targetTube = static_cast<int>(message.eTubeNum());

        if (!IsMessageForThisTube(targetTube)) {
            return;
        }

        m_launchtubemanager->UpdateLoadedWeaponKind(
            static_cast<EN_WPN_KIND>(message.eWpnKind())
        );
    }

    void TubeMessageReceiver::OnAssignCommandReceived(const TEWA_ASSIGN_CMD& message) {
        int targetTube = static_cast<int>(message.stWpnAssign().enTubeNum());

        if (!IsMessageForThisTube(targetTube)) {
            return;
        }

        DEBUG_STREAM(MESSAGERECEIVER) << "Tube " << m_tubeNumber << " received TEWA_ASSIGN_CMD" << std::endl;

        bool success{ false };

        try {
           if (message.eSetCmd() == static_cast<uint32_t>(EN_SET_CMD::SET_CMD_SET)) {
                success = m_launchtubemanager->AssignWeapon(message);
                if (success) {
                    AIEP_ASSIGN_RESP RespMsg;
                    RespMsg.eSetCmd() = static_cast<uint32_t>(EN_SET_CMD::SET_CMD_SET);
                    RespMsg.stWpnAssign() = message.stWpnAssign();
                    RespMsg.stMsgHeader().eTopicID() = static_cast<int32_t>(EN_TOPIC_ID::TOPIC_ID_AIEP_ASSIGN_RESP);
                    m_ddsComm->Send(RespMsg);
                    DEBUG_STREAM(MESSAGERECEIVER) << "Sucessfully Sent: AIEP_ASSIGN_RESP - SET" << std::endl;
                }
                else
                {
                    AIEP_ASSIGN_RESP RespMsg;
                    RespMsg.eSetCmd() = static_cast<uint32_t>(EN_SET_CMD::SET_CMD_REJECTED);
                    RespMsg.stMsgHeader().eTopicID() = static_cast<int32_t>(EN_TOPIC_ID::TOPIC_ID_AIEP_ASSIGN_RESP);
                    RespMsg.stWpnAssign() = message.stWpnAssign();

                    m_ddsComm->Send(RespMsg);
                    DEBUG_ERROR_STREAM(MESSAGERECEIVER) << "Rejected to assign weapon" << std::endl;
                }
            }
            else if (message.eSetCmd() == static_cast<uint32_t>(EN_SET_CMD::SET_CMD_UNSET)) {
                success = m_launchtubemanager->UnassignWeapon();
                if (success) {
                    AIEP_ASSIGN_RESP RespMsg;
                    RespMsg.eSetCmd() = static_cast<uint32_t>(EN_SET_CMD::SET_CMD_UNSET);
                    RespMsg.stMsgHeader().eTopicID() = static_cast<int32_t>(EN_TOPIC_ID::TOPIC_ID_AIEP_ASSIGN_RESP);
                    RespMsg.stWpnAssign() = message.stWpnAssign();

                    m_ddsComm->Send(RespMsg);
                    DEBUG_STREAM(MESSAGERECEIVER) << "Sucessfully Sent: AIEP_ASSIGN_RESP - UNSET" << std::endl;                    
                }
                else
                {
                    AIEP_ASSIGN_RESP RespMsg;
                    RespMsg.eSetCmd() = static_cast<uint32_t>(EN_SET_CMD::SET_CMD_REJECTED);
                    RespMsg.stWpnAssign() = message.stWpnAssign();
                    RespMsg.stMsgHeader().eTopicID() = static_cast<int32_t>(EN_TOPIC_ID::TOPIC_ID_AIEP_ASSIGN_RESP);
                    m_ddsComm->Send(RespMsg);
                    DEBUG_ERROR_STREAM(MESSAGERECEIVER) << "Rejected to unassign weapon" << std::endl;
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

    void TubeMessageReceiver::OnPAInfoReceived(const CMSHCI_AIEP_PA_INFO& message)
    {
        m_launchtubemanager->ProcessPAInfo(message);
    }

    void TubeMessageReceiver::OnWeaponControlCommandReceived(const CMSHCI_AIEP_WPN_CTRL_CMD& message)
    {
        int targetTube = static_cast<int>(message.eTubeNum());

        if (!IsMessageForThisTube(targetTube)) {
            return;
        }
        m_launchtubemanager->ProcessWeaponControlCommand(message);
    }

    void TubeMessageReceiver::OnWaypointsReceived(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& message)
    {
        int targetTube = static_cast<int>(message.eTubeNum());

        if (!IsMessageForThisTube(targetTube)) {
            return;
        }
        m_launchtubemanager->ProcessWaypointCommand(message);
    }

    void TubeMessageReceiver::OnOwnShipInfoReceived(const NAVINF_SHIP_NAVIGATION_INFO& message)
    {
        m_launchtubemanager->ProcessOwnshipInfo(message);
    }

    void TubeMessageReceiver::OnAIWaypointsInferenceRequestReceived(const CMSHCI_AIEP_AI_WAYPOINTS_INFERENCE_REQ& message)
    {
        int targetTube = static_cast<int>(message.eTubeNum());

        if (!IsMessageForThisTube(targetTube)) {
            return;
        }
        m_launchtubemanager->ProcessAIWaypointsInferenceRequest(message);
    }


    void TubeMessageReceiver::OnAIWaypointsInferenceResultReceived(const AIEP_INTERNAL_INFER_RESULT_WP& message)
    {
        int targetTube = static_cast<int>(message.enTubeNum());

        if (!IsMessageForThisTube(targetTube)) {
            return;
        }
        m_launchtubemanager->ProcessAIWaypointsInferenceResult(message);
    }

    void TubeMessageReceiver::OnSystemTargetInfoReceived(const TRKMGR_SYSTEMTARGET_INFO& message)
    {
        m_launchtubemanager->ProcessSystemTargetInfo(message);
    }
} // namespace AIEP
