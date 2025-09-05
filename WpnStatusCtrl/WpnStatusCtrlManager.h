#include "WpnStatusCtrlManager.h"
#include "../Common/Utils/ConfigManager.h"

namespace AIEP {

    // 무장 상태 전이 규칙
    const std::map<EN_WPN_CTRL_STATE, std::set<EN_WPN_CTRL_STATE>>
        WpnStatusCtrlManager::s_validTransitions = {
            {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON}},
            {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
            {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_RTL, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH, EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
            {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT}},
            {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
            {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POST_LAUNCH, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}}
        };

    WpnStatusCtrlManager::WpnStatusCtrlManager(int tubeNumber, uint32_t weaponKind, std::shared_ptr<AIEP::DdsComm> ddsComm)
        : m_tubeNumber(tubeNumber)
        , m_weaponKind(weaponKind)
        , m_ddsComm(ddsComm)
        , m_currentState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF)
        , m_initialized(false)
        , m_shutdown(false)
    {
        if (!ddsComm) {
            throw std::invalid_argument("DdsComm cannot be null");
        }

        m_launchInProgress.store(false);
        m_initialized.store(true);

        m_wpnStatusCtrlThread = std::thread([this]() {WorkerLoop();}); // 생성과 동시에 loop 시작

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

        if (m_wpnStatusCtrlThread.joinable()) {
            m_wpnStatusCtrlThread.join();
        }

        int waitCount{ 0 };

        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_currentState.store(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
        }
        // 콜백 함수들 정리
        {
           std::lock_guard<std::mutex> lock(m_callbackMutex);
            m_isEngagementPlanReady = nullptr;
            m_onWeaponLaunched = nullptr;
        }
        m_rtlCheckActive.store(false);
        m_launchInProgress.store(false);
        m_initialized.store(false);
    }

    void WpnStatusCtrlManager::WorkerLoop()
    {
        auto lastStatusSend = std::chrono::steady_clock::now();

        DEBUG_STREAM(WEAPONSTATE) << "WorkerLoop started for Tube " << m_tubeNumber << std::endl;

        while (!m_shutdown.load()) {
            auto now = std::chrono::steady_clock::now();

            // 1초마다 상태 송신
            if (now - lastStatusSend >= std::chrono::seconds(1)) {
                SendWeaponStatus();
                lastStatusSend = now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        DEBUG_STREAM(WEAPONSTATE) << "WorkerLoop ended for Tube " << m_tubeNumber << std::endl;
    }

    bool WpnStatusCtrlManager::ProcessControlCommand(const CMSHCI_AIEP_WPN_CTRL_CMD& command) {
        if (!m_initialized.load()) {
            DEBUG_ERROR_STREAM(WEAPONSTATE) << "WpnStatusCtrlManager not initialized" << std::endl;
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

    // LaunchTubeManager가 교전계획 준비 상태 체크 함수를 주입
    void WpnStatusCtrlManager::SetEngagementPlanChecker(std::function<bool()> checker) {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_isEngagementPlanReady = checker;
        DEBUG_STREAM(WEAPONSTATE) << "Engagement plan checker function injected for Tube " << m_tubeNumber << std::endl;
    }

    // LaunchTubeManager가 발사 완료 알림 콜백을 주입
    void WpnStatusCtrlManager::SetLaunchCompletedNotifier(std::function<void(std::chrono::steady_clock::time_point)> notifier) {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_onWeaponLaunched = notifier;
        DEBUG_STREAM(WEAPONSTATE) << "Launch completed notifier function injected for Tube " << m_tubeNumber << std::endl;
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

        // 무장 켬 후 경과 시간 저장
        if (targetState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON)
        {
            m_weaponOnTime = std::chrono::steady_clock::now();
            m_isWeaponOn.store(true);

            // ON 상태가 되면 RTL 전이 체크 시작
            CheckForRTLTransition();
        }
        else if (targetState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF) 
        {
            m_isWeaponOn.store(false);
            m_rtlCheckActive.store(false);

        }
        // 발사 명령인 경우 발사 절차 시작
        else if (targetState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH) {
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

    void WpnStatusCtrlManager::ChangeWeaponState(EN_WPN_CTRL_STATE newState) {
        EN_WPN_CTRL_STATE previousState = m_currentState.exchange(newState);

        DEBUG_STREAM(WEAPONSTATE) << "Tube " << m_tubeNumber << " state changed: "
            << static_cast<int>(previousState) << " -> " << static_cast<int>(newState) << std::endl;
    }

    void WpnStatusCtrlManager::CheckForRTLTransition() {
        // 이미 RTL 체크가 활성화되어 있으면 무시
        if (m_rtlCheckActive.exchange(true)) {
            return;
        }

        std::thread([this]() {
            DEBUG_STREAM(WEAPONSTATE) << "RTL transition check started for Tube " << m_tubeNumber << std::endl;

            while (m_currentState.load() == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON &&
                !m_shutdown.load() &&
                m_rtlCheckActive.load()) {

                // 콜백 함수로 교전계획 준비 상태 확인
                bool planReady = false;
                {
                    std::lock_guard<std::mutex> lock(m_callbackMutex);
                    if (m_isEngagementPlanReady) 
                    {
                        planReady = m_isEngagementPlanReady();
                    }
                }

                if (planReady) {
                    // 교전계획이 준비되었으면 RTL로 전이
                    ChangeWeaponState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_RTL);
                    DEBUG_STREAM(WEAPONSTATE) << "Auto transition to RTL for Tube " << m_tubeNumber << std::endl;
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            m_rtlCheckActive.store(false);
            DEBUG_STREAM(WEAPONSTATE) << "RTL transition check ended for Tube " << m_tubeNumber << std::endl;
            }).detach();
    }

    void WpnStatusCtrlManager::SendWeaponStatus() {
        if (!m_ddsComm || m_shutdown.load()) {
            return;
        }

        try {
            AIEP_WPN_CTRL_STATUS_INFO statusMsg;

            // 기본 정보 설정
            statusMsg.eTubeNum() = static_cast<uint32_t>(m_tubeNumber);
            statusMsg.eCtrlState() = static_cast<uint32_t>(m_currentState.load());

            // 무장 켠 후 경과시간 계산
            if (m_isWeaponOn.load()) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_weaponOnTime);
                statusMsg.wpnTime() = static_cast<uint32_t>(elapsed.count());
            }
            else {
                statusMsg.wpnTime() = 0;
            }

            // DDS로 메시지 전송
            m_ddsComm->Send(statusMsg);

            DEBUG_STREAM(WEAPONSTATE) << "Weapon status sent for Tube " << m_tubeNumber
                << ", State: " << static_cast<int>(m_currentState.load()) << std::endl;
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(WEAPONSTATE) << "Failed to send weapon status: " << e.what() << std::endl;
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
            auto launchTime = std::chrono::steady_clock::now();
            // 콜백 함수로 발사 완료 알림
            {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                if (m_onWeaponLaunched) {
                    m_onWeaponLaunched(launchTime);
                    DEBUG_STREAM(WEAPONSTATE) << "Launch completed callback invoked for Tube " << m_tubeNumber << std::endl;
                }
            }
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(WEAPONSTATE) << "Exception in launch sequence: " << e.what() << std::endl;
            m_launchInProgress.store(false);
            m_abortRequested.store(false);
        }
    }
} // namespace AIEP
