#pragma once

#include "../../Common/Types/CommonTypes.h"
#include "../../dds_message/AIEP_AIEP_.hpp"
#include <functional>
#include <memory>
#include <set>
#include <map>

namespace WeaponControl {

// 전방 선언
class IWeapon;

/**
 * @brief 무장 상태 관리자 인터페이스
 * 
 * 무장의 상태 전이, 인터록 조건 확인, 발사 절차 등을 관리합니다.
 * IWeapon과 분리되어 상태 관리 로직만을 담당합니다.
 */
class IWeaponStateManager {
public:
    virtual ~IWeaponStateManager() = default;
    
    // ==========================================================================
    // 초기화 및 설정
    // ==========================================================================
    
    /**
     * @brief 상태 관리자 초기화
     * @param tubeNumber 발사관 번호
     * @param weaponKind 무장 종류
     * @return 성공 여부
     */
    virtual bool Initialize(int tubeNumber, EN_WPN_KIND weaponKind) = 0;
    
    /**
     * @brief 관리할 무장 객체 설정
     * @param weapon 무장 객체 포인터
     */
    virtual void SetWeapon(std::weak_ptr<IWeapon> weapon) = 0;
    
    /**
     * @brief 상태 관리자 리셋
     */
    virtual void Reset() = 0;
    
    // ==========================================================================
    // 상태 관리
    // ==========================================================================
    
    /**
     * @brief 현재 상태 반환
     */
    virtual EN_WPN_CTRL_STATE GetCurrentState() const = 0;
    
    /**
     * @brief 상태 변경 요청
     * @param newState 요청된 새 상태
     * @return 성공 여부
     */
    virtual bool RequestStateChange(EN_WPN_CTRL_STATE newState) = 0;
    
    /**
     * @brief 상태 전이 유효성 확인
     * @param from 현재 상태
     * @param to 목표 상태
     * @return 유효한 전이인지 여부
     */
    virtual bool IsValidTransition(EN_WPN_CTRL_STATE from, EN_WPN_CTRL_STATE to) const = 0;
    
    /**
     * @brief 강제 중단 요청 (ABORT 명령)
     * @return 성공 여부
     */
    virtual bool RequestAbort() = 0;
    
    // ==========================================================================
    // 인터록 관리
    // ==========================================================================
    
    /**
     * @brief 인터록 조건 확인
     * @return 인터록 조건 만족 여부
     */
    virtual bool CheckInterlockConditions() const = 0;
    
    /**
     * @brief 인터록 상태 설정 (외부에서 설정)
     * @param status 인터록 상태
     */
    virtual void SetInterlockStatus(bool status) = 0;
    
    /**
     * @brief 교전계획 준비 상태 설정
     * @param ready 교전계획 준비 상태
     */
    virtual void SetEngagementPlanReady(bool ready) = 0;
    
    // ==========================================================================
    // 주기적 업데이트
    // ==========================================================================
    
    /**
     * @brief 상태 관리자 업데이트
     */
    virtual void Update() = 0;
    
    // ==========================================================================
    // 콜백 등록
    // ==========================================================================
    
    /**
     * @brief 상태 변경 콜백 등록
     * @param callback 상태 변경 시 호출될 콜백 함수
     */
    virtual void SetStateChangeCallback(std::function<void(EN_WPN_CTRL_STATE, EN_WPN_CTRL_STATE)> callback) = 0;
    
    /**
     * @brief 발사 상태 변경 콜백 등록
     * @param callback 발사 상태 변경 시 호출될 콜백 함수
     */
    virtual void SetLaunchStatusCallback(std::function<void(bool)> callback) = 0;
    
    // ==========================================================================
    // 상태 정보 조회
    // ==========================================================================
    
    /**
     * @brief 발사 준비 완료 상태 확인
     * @return RTL(Ready To Launch) 상태인지 여부
     */
    virtual bool IsReadyToLaunch() const = 0;
    
    /**
     * @brief 발사 중 상태 확인
     * @return 발사 중인지 여부
     */
    virtual bool IsLaunching() const = 0;
    
    /**
     * @brief 발사 완료 상태 확인
     * @return 발사 완료되었는지 여부
     */
    virtual bool IsPostLaunch() const = 0;
    
    /**
     * @brief 상태 시작 시간 반환
     * @return 현재 상태가 시작된 시간
     */
    virtual std::chrono::steady_clock::time_point GetStateStartTime() const = 0;
    
    /**
     * @brief 현재 상태 지속 시간 반환
     * @return 현재 상태가 시작된 후 경과 시간
     */
    virtual std::chrono::milliseconds GetStateDuration() const = 0;
};

} // namespace WeaponControl
