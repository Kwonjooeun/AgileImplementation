#pragma once

#include "IWeaponStateManager.h"
#include "../Weapons/IWeapon.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <condition_variable>

namespace WeaponControl {

// 발사 단계 정보
struct LaunchStep {
    std::string description;
    float duration_sec;
    
    LaunchStep(const std::string& desc, float duration) 
        : description(desc), duration_sec(duration) {}
};

/**
 * @brief 무장 상태 관리자 기본 클래스
 * 
 * IWeaponStateManager의 공통 기능을 구현하는 기반 클래스입니다.
 * 파생 클래스에서 무장별 특화 로직을 구현할 수 있습니다.
 */
class WeaponStateManagerBase : public IWeaponStateManager {
public:
    explicit WeaponStateManagerBase(EN_WPN_KIND weaponKind);
    virtual ~WeaponStateManagerBase();
    
    // ==========================================================================
    // IWeaponStateManager 인터페이스 구현
    // ==========================================================================
    
    bool Initialize(int tubeNumber, EN_WPN_KIND weaponKind) override;
    void SetWeapon(std::weak_ptr<IWeapon> weapon) override;
    void Reset() override;
    
    EN_WPN_CTRL_STATE GetCurrentState() const override;
    bool RequestStateChange(EN_WPN_CTRL_STATE newState) override;
    bool IsValidTransition(EN_WPN_CTRL_STATE from, EN_WPN_CTRL_STATE to) const override;
    bool RequestAbort() override;
    
    bool CheckInterlockConditions() const override;
    void SetInterlockStatus(bool status) override;

    void SetLaunchStatusCallback(std::function<void(bool)> callback) override;
    
    bool IsReadyToLaunch() const override;
    bool IsLaunching() const override;
    bool IsPostLaunch() const override;

    std::chrono::milliseconds GetOnStateDuration() const override;

protected:
    // ==========================================================================
    // 파생 클래스용 가상 함수들
    // ==========================================================================
    
    /**
     * @brief 상태 전이 맵 반환 (파생 클래스에서 오버라이드 가능)
     */
    virtual std::map<EN_WPN_CTRL_STATE, std::set<EN_WPN_CTRL_STATE>> GetValidTransitionMap() const;
    
    /**
     * @brief 무장별 초기화 처리
     */
    virtual bool OnInitialize() { return true; }
    
    /**
     * @brief 상태 진입 시 처리
     * @param state 진입하는 상태
     */
    virtual void OnStateEnter(EN_WPN_CTRL_STATE state) {}
    
    /**
     * @brief 상태 종료 시 처리
     * @param state 종료하는 상태
     */
    virtual void OnStateExit(EN_WPN_CTRL_STATE state) {}
    
    /**
     * @brief 상태별 주기적 처리
     * @param state 현재 상태
     */
    virtual void OnStateUpdate(EN_WPN_CTRL_STATE state) {}
    
    /**
     * @brief 무장별 인터록 조건 확인
     */
    virtual bool CheckWeaponSpecificInterlocks() const { return true; }
    
    /**
     * @brief 발사 단계 목록 반환 (파생 클래스에서 오버라이드)
     */
    virtual std::vector<LaunchStep> GetLaunchSteps() const;
    
    /**
     * @brief Power-On Check 지속 시간 반환
     */
    virtual float GetPowerOnCheckDuration() const;
    
    // ==========================================================================
    // 유틸리티 함수들
    // ==========================================================================
    
    /**
     * @brief 상태 변경 (내부용)
     * @param newState 새 상태
     */
    void SetState(EN_WPN_CTRL_STATE newState);
    
    /**
     * @brief 취소 확인과 함께 대기
     * @param duration 대기 시간 (초)
     * @return 정상 완료 여부 (false면 취소됨)
     */
    bool SleepWithCancellationCheck(float duration);
    
    /**
     * @brief 무장 객체 접근 (thread-safe)
     */
    std::shared_ptr<IWeapon> GetWeapon() const;
    
    /**
     * @brief 콜백 알림
     */
    void NotifyLaunchStatusChanged(bool launched);

private:
    // ==========================================================================
    // 상태 전이 처리 함수들
    // ==========================================================================
    
    bool ProcessTurnOn();
    bool ProcessTurnOff();
    bool ProcessLaunch();
    bool ProcessAbortInternal();
    
    // ==========================================================================
    // 스레드 관련 함수들
    // ==========================================================================
    
    void LaunchSequenceThread();
    void CancelCurrentOperation();
    
    // ==========================================================================
    // 기본 멤버 변수들
    // ==========================================================================
    
    EN_WPN_KIND m_weaponKind;
    int m_tubeNumber;
    std::weak_ptr<IWeapon> m_weapon;
    
    // 상태 관련
    std::atomic<EN_WPN_CTRL_STATE> m_currentState;
    std::chrono::steady_clock::time_point m_stateStartTime;
    
    // 인터록 관련
    std::atomic<bool> m_interlockStatus;
    std::atomic<bool> m_engagementPlanReady;
    
    // 취소 관련
    std::atomic<bool> m_cancelRequested;
    std::condition_variable m_cancelCondition;
    std::mutex m_cancelMutex;
    
    // 발사 관련
    std::unique_ptr<std::thread> m_launchThread;
    std::atomic<bool> m_launchInProgress;
    
    // 콜백 관련
    std::function<void(bool)> m_launchStatusCallback;
    mutable std::mutex m_callbackMutex;
    
    // 동기화
    mutable std::mutex m_stateMutex;
    mutable std::mutex m_weaponMutex;
    
    // 초기화 상태
    bool m_initialized;
    
    // 기본 상태 전이 맵 (static)
    static const std::map<EN_WPN_CTRL_STATE, std::set<EN_WPN_CTRL_STATE>> s_defaultTransitionMap;
};

} // namespace WeaponControl
