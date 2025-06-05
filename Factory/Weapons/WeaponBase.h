#pragma once

#include "IWeapon.h"
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>

namespace WeaponControl {

/**
 * @brief 무장 기본 클래스
 * 
 * IWeapon 인터페이스의 공통 기능을 구현하는 기반 클래스입니다.
 * 상태 관리 기능은 제거되고 순수한 무장 속성과 기본 동작만 제공합니다.
 */
class WeaponBase : public IWeapon {
public:
    explicit WeaponBase(EN_WPN_KIND weaponKind);
    virtual ~WeaponBase() = default;
    
    // ==========================================================================
    // IWeapon 인터페이스 구현
    // ==========================================================================
    
    EN_WPN_KIND GetWeaponKind() const override { return m_weaponKind; }
    WeaponSpec GetSpecification() const override;
    int GetTubeNumber() const override { return m_tubeNumber; }
    
    bool Initialize(int tubeNumber) override;
    void Reset() override;
    void Update() override;
    
    bool IsLaunched() const override { return m_launched.load(); }
    void SetLaunched(bool launched) override;
    
    bool IsFireSolutionReady() const override { return m_fireSolutionReady.load(); }
    void SetFireSolutionReady(bool ready) override;
    
    bool IsSystemReady() const override;
    
    void SetLaunchStatusCallback(std::function<void(bool)> callback) override;
    void SetSystemReadyCallback(std::function<void(bool)> callback) override;

protected:
    // ==========================================================================
    // 파생 클래스용 가상 함수들
    // ==========================================================================
    
    /**
     * @brief 무장별 초기화 처리
     */
    virtual bool OnInitialize() { return true; }
    
    /**
     * @brief 무장별 리셋 처리
     */
    virtual void OnReset() {}
    
    /**
     * @brief 무장별 업데이트 처리
     */
    virtual void OnUpdate() {}
    
    /**
     * @brief 무장별 사양 정보 반환
     */
    virtual WeaponSpec CreateSpecification() const = 0;
    
    /**
     * @brief 무장별 시스템 준비 상태 확인
     */
    virtual bool CheckSystemReady() const { return true; }
    
    // ==========================================================================
    // 콜백 알림 헬퍼 함수들
    // ==========================================================================
    
    void NotifyLaunchStatusChanged(bool launched);
    void NotifySystemReadyChanged(bool ready);
    
    // ==========================================================================
    // 멤버 변수 접근자들
    // ==========================================================================
    
    EN_WPN_KIND GetWeaponKindInternal() const { return m_weaponKind; }
    int GetTubeNumberInternal() const { return m_tubeNumber; }
    
private:
    // ==========================================================================
    // 기본 멤버 변수들
    // ==========================================================================
    
    EN_WPN_KIND m_weaponKind;
    int m_tubeNumber;
    
    // 상태 관련 (원자적 접근)
    std::atomic<bool> m_launched;
    std::atomic<bool> m_fireSolutionReady;
    std::atomic<bool> m_systemReady;
    
    // 초기화 상태
    bool m_initialized;
    
    // 콜백 함수들
    std::function<void(bool)> m_launchStatusCallback;
    std::function<void(bool)> m_systemReadyCallback;
    
    // 동기화
    mutable std::mutex m_callbackMutex;
    mutable std::mutex m_initMutex;
    
    // 타이밍 정보
    std::chrono::steady_clock::time_point m_initTime;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    
    // 캐시된 사양 정보
    mutable WeaponSpec m_cachedSpec;
    mutable bool m_specCached;
    mutable std::mutex m_specMutex;
};

} // namespace WeaponControl
