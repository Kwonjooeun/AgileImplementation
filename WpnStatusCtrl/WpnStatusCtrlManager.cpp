#include "WpnStatusCtrlManager.h"
#include "../Common/Utils/ConfigManager.h"

namespace MINEASMALM {

    // 무장 상태 전이 규칙 (문서에서 제공된 규칙)
    const std::map<EN_WPN_CTRL_STATE, std::set<EN_WPN_CTRL_STATE>> 
    WpnStatusCtrlManager::s_validTransitions = {
        {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON}},
        {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
        {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_RTL, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH, EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
        {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT}},
        {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
        {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POST_LAUNCH, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}}
    };

    WpnStatusCtrlManager::WpnStatusCtrlManager(int tubeNumber, EN_WPN_KIND weaponKind, std::shared_ptr<DdsComm> ddsComm)
        : m_tubeNumber(tubeNumber)
        , m_weaponKind(weaponKind)
        , m_ddsComm(ddsComm)
        , m_currentState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF)
        , m_initialized(false)
        , m_shutdown(false)
        , m_launchInProgress(false)
    {
        if (!ddsComm) {
            throw std::invalid_argument("DdsComm cannot be null");
        }
    
        m_shutdown.store(false);
        m_launchInProgress.store(false);
        m_initialized.store(true);
    
        DEBUG_STREAM(WEAPONSTATE) << "WpnStatusCtrlManager initialized for Tube " << m_tubeNumber 
                                  << ", Weapon Kind: " << static_cast<int>(m_weaponKind) << std::endl;
    }

    WpnStatusCtrlManager::~WpnStatusCtrlManager() {
        Shutdown();
    }

    void WpnStatusCtrlManager::Shutdown() {
        if (!m_initialized.load()) {
            return;
        }

        DEBUG_STREAM(WEAPONSTATE) << "WpnStatusCtrlManager shutdown for Tube " << m_tubeNumber << std::endl;

        m_shutdown.store(true);
        m_abortRequested.store(true);

        int waitCount{ 0 };

        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_currentState.store(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
        }

        m_launchInProgress.store(false);
        m_initialized.store(false);
    }

    void WpnStatusCtrlManager::WorkerLoop()
    {
        auto lastStatusSend = std::chrono::steady_clock::now();

        DEBUG_STREAM(WEAPONSTATE) << "WorkerLoop started for Tube " << m_tubeNumber << std::endl;

        while (!m_shutdown.load()) {
            auto now = std::chrono::steady_clock::now();

            // 1초마다 상태 송신 (mutex로 상태 보호)
            if (now - lastStatusSend >= std::chrono::seconds(1)) {
                SendWeaponStatus();
                lastStatusSend = now;
            }
        }

        DEBUG_STREAM(WEAPONSTATE) << "WorkerLoop ended for Tube " << m_tubeNumber << std::endl;
    }

    bool WpnStatusCtrlManager::ProcessControlCommand(const CMSHCI_AIEP_WPN_CTRL_CMD& command) {
        if (!m_initialized.load()) {
            DEBUG_ERROR_STREAM(WEAPONSTATE) << "WpnStatusCtrlManager not initialized" << std::endl;
            return false;
        }

        // 무장 종류 확인
        if (static_cast<EN_WPN_KIND>(command.eWpnKind()) != m_weaponKind) {
            DEBUG_ERROR_STREAM(WEAPONSTATE) << "Weapon kind mismatch. Expected: "
                << static_cast<int>(m_weaponKind)
                << ", Received: " << command.eWpnKind() << std::endl;
            return false;
        }

        // 별도 스레드에서 상태 전이 처리
        std::thread([this, command]() {
            ProcessControlCommandInternal(command);
            }).detach();

        return true;
    }

    EN_WPN_CTRL_STATE WpnStatusCtrlManager::GetCurrentState() const {
        return m_currentState.load();
    }

    bool WpnStatusCtrlManager::IsValidTransition(EN_WPN_CTRL_STATE fromState, EN_WPN_CTRL_STATE toState) const {
        auto it = s_validTransitions.find(fromState);
        if (it == s_validTransitions.end()) {
            return false;
        }

        return it->second.find(toState) != it->second.end();
    }

    void WpnStatusCtrlManager::ProcessControlCommandInternal(const CMSHCI_AIEP_WPN_CTRL_CMD& command) 
    {
        EN_WPN_CTRL_STATE targetState = static_cast<EN_WPN_CTRL_STATE>(command.eWpnCtrlCmd());

        // ABORT 명령은 즉시 처리
        if (targetState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT) 
        {
            m_abortRequested.store(true);  // 발사 절차 중단 신호
            DEBUG_STREAM(WEAPONSTATE) << "ABORT " << std::endl;
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_currentState.store(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_stateMutex);

            EN_WPN_CTRL_STATE currentState = m_currentState.load();
            // 동일 상태면 성공으로 처리
            if (currentState == targetState) {
                DEBUG_STREAM(WEAPONSTATE) << "Already in target state: " << static_cast<int>(targetState) << std::endl;
                return;
            }

            // 상태 전이 유효성 검사
            if (!IsValidTransition(currentState, targetState)) {
                DEBUG_ERROR_STREAM(WEAPONSTATE) << "Invalid state transition: "
                    << static_cast<int>(currentState)
                    << " -> " << static_cast<int>(targetState) << std::endl;
                return;
            }
            m_currentState.store(targetState);
        }
        
        if (targetState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON) 
        {
            m_weaponOnTime = std::chrono::steady_clock::now();
            m_isWeaponOn.store(true);
        }
        else if (targetState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF) 
        {
            m_isWeaponOn.store(false);
        }
        
        // 발사 명령인 경우 발사 절차 시작
        if (targetState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH) {
            if (m_launchInProgress.exchange(true)) {
                DEBUG_STREAM(WEAPONSTATE) << "Launch already in progress" << std::endl;
                return;
            }
            std::thread([this]() { ProcessLaunchSequence(); }).detach();  // detached로 실행
        }
        else {
            // 일반 상태 전이
        }
    }

    void WpnStatusCtrlManager::ProcessLaunchSequence() {
        try {
            // ConfigManager에서 무장 스펙 조회
            auto& configMgr = ConfigManager::GetInstance();
            const auto& weaponSpec = configMgr.GetWeaponSpec(m_weaponKind);

            double launchDelay_sec = weaponSpec.launchDelay_sec;

            DEBUG_STREAM(WEAPONSTATE) << "Starting launch sequence for Tube " << m_tubeNumber 
                                      << ", Launch delay: " << launchDelay_sec << " seconds" << std::endl;

            // 발사 지연시간만큼 대기
            auto startTime = std::chrono::steady_clock::now();
            auto delayDuration = std::chrono::duration<double>(launchDelay_sec);


            while (!m_shutdown.load() && !m_abortRequested.load()) 
            {
                auto elapsed = std::chrono::steady_clock::now() - startTime;
                if (elapsed >= delayDuration) {
                    break;
                }

                // 짧은 간격으로 shutdown || abort 확인
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Shutdown or Abort이면 발사 절차 중단
            if (m_shutdown.load() || m_abortRequested.load()) 
            {
                DEBUG_STREAM(WEAPONSTATE) << "Launch sequence aborted due to shutdown" << std::endl;
                m_launchInProgress.store(false);
                m_abortRequested.store(false); // 플래그 리셋
                return;
            }

            // 발사 완료 - POST_LAUNCH 상태로 전이
            {
                std::lock_guard<std::mutex> lock(m_stateMutex);
                m_currentState.store(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POST_LAUNCH);
                m_launchInProgress.store(false);

                DEBUG_STREAM(WEAPONSTATE) << "Launch sequence completed for Tube " << m_tubeNumber
                    << ", State transitioned to POST_LAUNCH" << std::endl;
            }
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(WEAPONSTATE) << "Exception in launch sequence: " << e.what() << std::endl;
            m_launchInProgress.store(false);
            m_abortRequested.store(false);
        }
    }

    void WpnStatusCtrlManager::SendWeaponStatus() {
        if (!m_ddsComm || m_shutdown.load()) {
            return;
        }
    
        try {
            AIEP_WPN_CTRL_STATUS_INFO statusMsg;
            
            statusMsg.eTubeNum() = static_cast<uint32_t>(m_tubeNumber);
            statusMsg.eCtrlState() = static_cast<uint32_t>(m_currentState.load());
            
            // 무장 켠 후 경과시간 계산
            if (m_isWeaponOn.load()) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_weaponOnTime);
                statusMsg.wpnTime() = static_cast<uint32_t>(elapsed.count());
            } else {
                statusMsg.wpnTime() = 0;
            }
            
            m_ddsComm->Send(statusMsg);
            
            DEBUG_STREAM(WEAPONSTATE) << "Weapon status sent for Tube " << m_tubeNumber 
                                      << ", State: " << static_cast<int>(m_currentState.load()) << std::endl;
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(WEAPONSTATE) << "Failed to send weapon status: " << e.what() << std::endl;
        }
    }
} // namespace MINEASMALM
