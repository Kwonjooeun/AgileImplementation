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

    WpnStatusCtrlManager::WpnStatusCtrlManager()
        : m_tubeNumber(0)
        , m_weaponKind(EN_WPN_KIND::WPN_KIND_NA)
        , m_currentState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF)
        , m_initialized(false)
        , m_shutdown(false)
        , m_launchInProgress(false)
    {}

    WpnStatusCtrlManager::~WpnStatusCtrlManager() {
        Shutdown();
    }

    bool WpnStatusCtrlManager::Initialize(int tubeNumber, EN_WPN_KIND weaponKind) {
        std::lock_guard<std::mutex> lock(m_stateMutex);

        if (m_initialized.load()) {
            return true;
        }

        m_tubeNumber = tubeNumber;
        m_weaponKind = weaponKind;
        m_currentState.store(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
        m_shutdown.store(false);
        m_launchInProgress.store(false);
        m_initialized.store(true);

        DEBUG_STREAM(WEAPONSTATE) << "WpnStatusCtrlManager initialized for Tube " << m_tubeNumber 
                                  << ", Weapon Kind: " << static_cast<int>(m_weaponKind) << std::endl;

        return true;
    }

    void WpnStatusCtrlManager::Shutdown() {
        std::lock_guard<std::mutex> lock(m_stateMutex);

        if (!m_initialized.load()) {
            return;
        }

        DEBUG_STREAM(WEAPONSTATE) << "WpnStatusCtrlManager shutdown for Tube " << m_tubeNumber << std::endl;

        m_shutdown.store(true);

        // 발사 스레드가 실행 중이면 대기
        if (m_launchThread.joinable()) {
            m_launchThread.join();
        }

        m_currentState.store(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
        m_launchInProgress.store(false);
        m_initialized.store(false);
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

        EN_WPN_CTRL_STATE targetState = static_cast<EN_WPN_CTRL_STATE>(command.eWpnCtrlCmd());

        DEBUG_STREAM(WEAPONSTATE) << "Processing control command for Tube " << m_tubeNumber 
                                  << ": " << static_cast<int>(m_currentState.load()) 
                                  << " -> " << static_cast<int>(targetState) << std::endl;

        return TransitionState(targetState);
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

    bool WpnStatusCtrlManager::TransitionState(EN_WPN_CTRL_STATE targetState) {
        std::lock_guard<std::mutex> lock(m_stateMutex);

        EN_WPN_CTRL_STATE currentState = m_currentState.load();

        // 동일 상태면 성공으로 처리
        if (currentState == targetState) {
            DEBUG_STREAM(WEAPONSTATE) << "Already in target state: " << static_cast<int>(targetState) << std::endl;
            return true;
        }

        // 상태 전이 유효성 검사
        if (!IsValidTransition(currentState, targetState)) {
            DEBUG_ERROR_STREAM(WEAPONSTATE) << "Invalid state transition: " 
                                            << static_cast<int>(currentState) 
                                            << " -> " << static_cast<int>(targetState) << std::endl;
            return false;
        }

        // 발사 명령인 경우 발사 절차 시작
        if (targetState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH) {
            if (m_launchInProgress.load()) {
                DEBUG_ERROR_STREAM(WEAPONSTATE) << "Launch already in progress" << std::endl;
                return false;
            }

            // 상태 업데이트
            m_currentState.store(targetState);
            m_launchInProgress.store(true);

            DEBUG_STREAM(WEAPONSTATE) << "State transition successful: " 
                                      << static_cast<int>(currentState) 
                                      << " -> " << static_cast<int>(targetState) << std::endl;

            // 발사 절차를 별도 스레드에서 처리
            if (m_launchThread.joinable()) {
                m_launchThread.join();
            }
            m_launchThread = std::thread(&WpnStatusCtrlManager::ProcessLaunchSequence, this);

            return true;
        }
        else {
            // 일반 상태 전이
            m_currentState.store(targetState);

            DEBUG_STREAM(WEAPONSTATE) << "State transition successful: " 
                                      << static_cast<int>(currentState) 
                                      << " -> " << static_cast<int>(targetState) << std::endl;

            return true;
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

            while (!m_shutdown.load()) {
                auto elapsed = std::chrono::steady_clock::now() - startTime;
                if (elapsed >= delayDuration) {
                    break;
                }

                // 짧은 간격으로 shutdown 확인
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Shutdown 중이면 발사 절차 중단
            if (m_shutdown.load()) {
                DEBUG_STREAM(WEAPONSTATE) << "Launch sequence aborted due to shutdown" << std::endl;
                m_launchInProgress.store(false);
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
        }
    }

} // namespace MINEASMALM
