#pragma once

#include "Weapons/IWeapon.h"
#include "EngagementManagers/IEngagementManager.h"
#include "StateManagers/IWeaponStateManager.h"
#include "../Common/Types/CommonTypes.h"
#include <memory>
#include <functional>
#include <map>

namespace WeaponControl {

// 무장 생성자 타입 정의
using WeaponCreator = std::function<std::unique_ptr<IWeapon>()>;
using EngagementManagerCreator = std::function<std::unique_ptr<IEngagementManager>()>;
using StateManagerCreator = std::function<std::unique_ptr<IWeaponStateManager>()>;

// 무장 사양 정보
struct WeaponSpecification {
    std::string name;
    double maxRange_km;
    double speed_mps;
    double launchDelay_sec;
    std::vector<std::string> supportedModes;
    
    WeaponSpecification() 
        : name(""), maxRange_km(0.0), speed_mps(0.0), launchDelay_sec(0.0) {}
    
    WeaponSpecification(const std::string& n, double range, double speed, double delay)
        : name(n), maxRange_km(range), speed_mps(speed), launchDelay_sec(delay) {}
};

/**
 * @brief 무장 관련 객체들을 생성하는 팩토리 클래스
 * 
 * 싱글톤 패턴을 사용하여 시스템 전체에서 일관된 무장 객체 생성을 담당합니다.
 * 각 무장 종류별로 Weapon, EngagementManager, StateManager를 생성할 수 있습니다.
 */
class WeaponFactory {
public:
    // 싱글톤 인스턴스 획득
    static WeaponFactory& GetInstance();
    
    // ==========================================================================
    // 객체 생성 메서드들
    // ==========================================================================
    
    /**
     * @brief 무장 객체 생성
     * @param weaponKind 무장 종류
     * @return 생성된 무장 객체 (실패시 nullptr)
     */
    std::unique_ptr<IWeapon> CreateWeapon(::EN_WPN_KIND weaponKind) const;
    
    /**
     * @brief 교전계획 관리자 생성
     * @param weaponKind 무장 종류
     * @return 생성된 교전계획 관리자 (실패시 nullptr)
     */
    std::unique_ptr<IEngagementManager> CreateEngagementManager(::EN_WPN_KIND weaponKind) const;
    
    /**
     * @brief 무장 상태 관리자 생성
     * @param weaponKind 무장 종류
     * @return 생성된 상태 관리자 (실패시 nullptr)
     */
    std::unique_ptr<IWeaponStateManager> CreateWeaponStateManager(::EN_WPN_KIND weaponKind) const;
    
    // ==========================================================================
    // 생성자 등록 (확장성을 위해)
    // ==========================================================================
    
    void RegisterWeaponCreator(EN_WPN_KIND weaponKind, WeaponCreator creator);
    void RegisterEngagementManagerCreator(EN_WPN_KIND weaponKind, EngagementManagerCreator creator);
    void RegisterStateManagerCreator(EN_WPN_KIND weaponKind, StateManagerCreator creator);
    
    // ==========================================================================
    // 정보 조회
    // ==========================================================================
    
    /**
     * @brief 지원되는 무장 종류 확인
     */
    bool IsWeaponSupported(EN_WPN_KIND weaponKind) const;
    
    /**
     * @brief 무장 사양 정보 제공
     */
    WeaponSpecification GetWeaponSpecification(EN_WPN_KIND weaponKind) const;
    
    /**
     * @brief 지원되는 모든 무장 종류 목록 반환
     */
    std::vector<EN_WPN_KIND> GetSupportedWeaponKinds() const;

private:
    WeaponFactory();
    ~WeaponFactory() = default;
    
    // 복사 및 이동 금지
    WeaponFactory(const WeaponFactory&) = delete;
    WeaponFactory& operator=(const WeaponFactory&) = delete;
    WeaponFactory(WeaponFactory&&) = delete;
    WeaponFactory& operator=(WeaponFactory&&) = delete;
    
    // 기본 생성자들 등록
    void RegisterDefaultCreators();
    
    // 생성자 맵들
    std::map<EN_WPN_KIND, WeaponCreator> m_weaponCreators;
    std::map<EN_WPN_KIND, EngagementManagerCreator> m_engagementManagerCreators;
    std::map<EN_WPN_KIND, StateManagerCreator> m_stateManagerCreators;
    
    // 무장 사양 정보
    std::map<EN_WPN_KIND, WeaponSpecification> m_weaponSpecs;
};

// =============================================================================
// 유틸리티 함수들
// =============================================================================

/**
 * @brief 무장 종류를 문자열로 변환
 */
inline std::string WeaponKindToString(EN_WPN_KIND kind) {
    switch(kind) {
        case EN_WPN_KIND::WPN_KIND_ALM: return "ALM";
        case EN_WPN_KIND::WPN_KIND_ASM: return "ASM";
        case EN_WPN_KIND::WPN_KIND_AAM: return "AAM";
        case EN_WPN_KIND::WPN_KIND_WGT: return "WGT";
        case EN_WPN_KIND::WPN_KIND_M_MINE: return "MINE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 무장 상태를 문자열로 변환
 */
inline std::string StateToString(EN_WPN_CTRL_STATE state) {
    switch(state) {
        case EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF: return "OFF";
        case EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POC: return "POC";
        case EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON: return "ON";
        case EN_WPN_CTRL_STATE::WPN_CTRL_STATE_RTL: return "RTL";
        case EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH: return "LAUNCH";
        case EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POST_LAUNCH: return "POST_LAUNCH";
        case EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT: return "ABORT";
        default: return "UNKNOWN";
    }
}

} // namespace WeaponControl
