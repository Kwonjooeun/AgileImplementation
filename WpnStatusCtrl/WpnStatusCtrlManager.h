#pragma once

#include "../dds_message/AIEP_AIEP_.hpp"
#include "../Common/Communication/DdsComm.h"
#include "../Common/Utils/DebugPrint.h"
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

namespace AIEP {

    // =============================================================================
    // 무장 상태 통제 관리자
    // =============================================================================
    class WpnStatusCtrlManager {
    public:
        WpnStatusCtrlManager(int tubeNumber, uint32_t weaponKind, std::shared_ptr<AIEP::DdsComm> ddsComm);
        ~WpnStatusCtrlManager();

        void Shutdown();
        void WorkerLoop();

        // 무장 상태 통제
        bool ProcessControlCommand(const CMSHCI_AIEP_WPN_CTRL_CMD& command);
        EN_WPN_CTRL_STATE GetCurrentState() const;

        // LaunchTubeManager가 교전계획 준비 상태 체크 함수를 주입
        void SetEngagementPlanChecker(std::function<bool()> checker);

        // LaunchTubeManager가 발사 완료 알림 콜백을 주입
        void SetLaunchCompletedNotifier(std::function<void(std::chrono::steady_clock::time_point)> notifier);
    private:
        // 상태 전이 규칙
        static const std::map<EN_WPN_CTRL_STATE, std::set<EN_WPN_CTRL_STATE>> s_validTransitions;

        // 콜백 함수들
        std::function<bool()> m_isEngagementPlanReady;
        std::function<void(std::chrono::steady_clock::time_point)> m_onWeaponLaunched;
        std::mutex m_callbackMutex;  // 콜백 함수 보호용

        // 상태 전이 확인
        bool IsValidTransition(EN_WPN_CTRL_STATE fromState, EN_WPN_CTRL_STATE toState) const;
        void ChangeWeaponState(EN_WPN_CTRL_STATE newState);

        // RTL 전이 체크 (백그라운드에서 실행)
        void CheckForRTLTransition();

        // 발사 절차 처리
        void ProcessLaunchSequence();

        // 멤버 변수
        int m_tubeNumber;
        uint32_t m_weaponKind;
        std::shared_ptr<AIEP::DdsComm> m_ddsComm;

        std::atomic<EN_WPN_CTRL_STATE> m_currentState;
        std::atomic<bool> m_initialized;
        std::atomic<bool> m_shutdown;
        std::atomic<bool> m_abortRequested{ false };

        std::chrono::steady_clock::time_point m_weaponOnTime;
        std::atomic<bool> m_isWeaponOn{ false };

        std::thread m_wpnStatusCtrlThread;

        // 발사 절차용
        std::atomic<bool> m_launchInProgress;

        // RTL 전이 체크 스레드 관리
        std::atomic<bool> m_rtlCheckActive{ false };

        mutable std::mutex m_stateMutex;

        void SendWeaponStatus();
        void ProcessControlCommandInternal(const CMSHCI_AIEP_WPN_CTRL_CMD& command);  // 실제 상태 전이 처리

    };

} // namespace AIEP
