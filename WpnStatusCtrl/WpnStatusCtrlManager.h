#pragma once

#include "../dds_message/AIEP_AIEP_.hpp"
#include "../Common/Utils/DebugPrint.h"
#include <map>
#include <set>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

namespace MINEASMALM {

    // =============================================================================
    // 무장 상태 통제 관리자
    // =============================================================================
    class WpnStatusCtrlManager {
    public:
        WpnStatusCtrlManager(int tubeNumber, EN_WPN_KIND weaponKind, std::shared_ptr<DdsComm> ddsComm);
        ~WpnStatusCtrlManager();

        // 종료
        void Shutdown();

        // 무장 상태 통제
        bool ProcessControlCommand(const CMSHCI_AIEP_WPN_CTRL_CMD& command);
        EN_WPN_CTRL_STATE GetCurrentState() const;

    private:
        std::shared_ptr<DdsComm> m_ddsComm;

        // 상태 전이 규칙 (문서에서 제공된 규칙)
        static const std::map<EN_WPN_CTRL_STATE, std::set<EN_WPN_CTRL_STATE>> s_validTransitions;

        // 상태 전이 처리
        bool TransitionState(EN_WPN_CTRL_STATE targetState);
        bool IsValidTransition(EN_WPN_CTRL_STATE fromState, EN_WPN_CTRL_STATE toState) const;

        // 발사 절차 처리
        void ProcessLaunchSequence();

        // 멤버 변수
        int m_tubeNumber;
        EN_WPN_KIND m_weaponKind;
        std::atomic<EN_WPN_CTRL_STATE> m_currentState;
        std::atomic<bool> m_initialized;
        std::atomic<bool> m_shutdown;
        
        // 발사 절차용
        std::atomic<bool> m_launchInProgress;
        std::thread m_launchThread;
        
        mutable std::mutex m_stateMutex;
    };

} // namespace MINEASMALM
