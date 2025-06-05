#include "WeaponFactory.h"
#include "Weapons/MineWeapon.h"
#include "Weapons/ALMWeapon.h"
#include "Weapons/ASMWeapon.h"
#include "EngagementManagers/MineEngagementManager.h"
#include "EngagementManagers/ALMEngagementManager.h"
#include "EngagementManagers/ASMEngagementManager.h"
#include "StateManagers/MineStateManager.h"
#include "StateManagers/MissileStateManager.h"
#include "../Common/Utils/ConfigManager.h"
#include <iostream>

namespace WeaponControl {

// =============================================================================
// 싱글톤 구현
// =============================================================================

WeaponFactory& WeaponFactory::GetInstance() {
    static WeaponFactory instance;
    return instance;
}

WeaponFactory::WeaponFactory() {
    std::cout << "WeaponFactory initializing..." << std::endl;
    RegisterDefaultCreators();
    std::cout << "WeaponFactory initialized with " << m_weaponCreators.size() << " weapon types" << std::endl;
}

// =============================================================================
// 객체 생성 메서드들
// =============================================================================

std::unique_ptr<IWeapon> WeaponFactory::CreateWeapon(EN_WPN_KIND weaponKind) const {
    auto it = m_weaponCreators.find(weaponKind);
    if (it != m_weaponCreators.end()) {
        auto weapon = it->second();
        if (weapon) {
            std::cout << "Created weapon: " << WeaponKindToString(weaponKind) << std::endl;
        }
        return weapon;
    }
    
    std::cerr << "Unsupported weapon kind: " << WeaponKindToString(weaponKind) << std::endl;
    return nullptr;
}

std::unique_ptr<IEngagementManager> WeaponFactory::CreateEngagementManager(EN_WPN_KIND weaponKind) const {
    auto it = m_engagementManagerCreators.find(weaponKind);
    if (it != m_engagementManagerCreators.end()) {
        auto manager = it->second();
        if (manager) {
            std::cout << "Created engagement manager for: " << WeaponKindToString(weaponKind) << std::endl;
        }
        return manager;
    }
    
    std::cerr << "Unsupported engagement manager for weapon: " << WeaponKindToString(weaponKind) << std::endl;
    return nullptr;
}

std::unique_ptr<IWeaponStateManager> WeaponFactory::CreateWeaponStateManager(EN_WPN_KIND weaponKind) const {
    auto it = m_stateManagerCreators.find(weaponKind);
    if (it != m_stateManagerCreators.end()) {
        auto manager = it->second();
        if (manager) {
            std::cout << "Created state manager for: " << WeaponKindToString(weaponKind) << std::endl;
        }
        return manager;
    }
    
    std::cerr << "Unsupported state manager for weapon: " << WeaponKindToString(weaponKind) << std::endl;
    return nullptr;
}

// =============================================================================
// 생성자 등록
// =============================================================================

void WeaponFactory::RegisterWeaponCreator(EN_WPN_KIND weaponKind, WeaponCreator creator) {
    m_weaponCreators[weaponKind] = creator;
    std::cout << "Registered weapon creator for: " << WeaponKindToString(weaponKind) << std::endl;
}

void WeaponFactory::RegisterEngagementManagerCreator(EN_WPN_KIND weaponKind, EngagementManagerCreator creator) {
    m_engagementManagerCreators[weaponKind] = creator;
    std::cout << "Registered engagement manager creator for: " << WeaponKindToString(weaponKind) << std::endl;
}

void WeaponFactory::RegisterStateManagerCreator(EN_WPN_KIND weaponKind, StateManagerCreator creator) {
    m_stateManagerCreators[weaponKind] = creator;
    std::cout << "Registered state manager creator for: " << WeaponKindToString(weaponKind) << std::endl;
}

// =============================================================================
// 정보 조회
// =============================================================================

bool WeaponFactory::IsWeaponSupported(EN_WPN_KIND weaponKind) const {
    return m_weaponCreators.find(weaponKind) != m_weaponCreators.end();
}

WeaponSpecification WeaponFactory::GetWeaponSpecification(EN_WPN_KIND weaponKind) const {
    auto it = m_weaponSpecs.find(weaponKind);
    if (it != m_weaponSpecs.end()) {
        return it->second;
    }
    
    std::cerr << "No specification found for weapon: " << WeaponKindToString(weaponKind) << std::endl;
    return WeaponSpecification();
}

std::vector<EN_WPN_KIND> WeaponFactory::GetSupportedWeaponKinds() const {
    std::vector<EN_WPN_KIND> supportedKinds;
    for (const auto& pair : m_weaponCreators) {
        supportedKinds.push_back(pair.first);
    }
    return supportedKinds;
}

// =============================================================================
// 기본 생성자들 등록
// =============================================================================

void WeaponFactory::RegisterDefaultCreators() {
    auto& config = ConfigManager::GetInstance();
    
    // ==========================================================================
    // 무장 생성자들 등록
    // ==========================================================================
    
    // ALM (잠대지 유도탄)
    RegisterWeaponCreator(EN_WPN_KIND::WPN_KIND_ALM, []() -> std::unique_ptr<IWeapon> {
        return std::make_unique<ALMWeapon>();
    });
    
    // ASM (잠대함 유도탄)
    RegisterWeaponCreator(EN_WPN_KIND::WPN_KIND_ASM, []() -> std::unique_ptr<IWeapon> {
        return std::make_unique<ASMWeapon>();
    });
    
    // AAM (잠대공 유도탄) - ASM과 유사한 구조로 ASMWeapon 재사용
    RegisterWeaponCreator(EN_WPN_KIND::WPN_KIND_AAM, []() -> std::unique_ptr<IWeapon> {
        return std::make_unique<ASMWeapon>(); // AAM은 ASM과 유사한 특성
    });
    
    // 자항기뢰
    RegisterWeaponCreator(EN_WPN_KIND::WPN_KIND_M_MINE, []() -> std::unique_ptr<IWeapon> {
        return std::make_unique<MineWeapon>();
    });
    
    // ==========================================================================
    // 교전계획 관리자 생성자들 등록
    // ==========================================================================
    
    RegisterEngagementManagerCreator(EN_WPN_KIND::WPN_KIND_ALM, []() -> std::unique_ptr<IEngagementManager> {
        return std::make_unique<ALMEngagementManager>();
    });
    
    RegisterEngagementManagerCreator(EN_WPN_KIND::WPN_KIND_ASM, []() -> std::unique_ptr<IEngagementManager> {
        return std::make_unique<ASMEngagementManager>();
    });
    
    RegisterEngagementManagerCreator(EN_WPN_KIND::WPN_KIND_AAM, []() -> std::unique_ptr<IEngagementManager> {
        return std::make_unique<ASMEngagementManager>(); // AAM도 ASM 관리자 재사용
    });
    
    RegisterEngagementManagerCreator(EN_WPN_KIND::WPN_KIND_M_MINE, []() -> std::unique_ptr<IEngagementManager> {
        return std::make_unique<MineEngagementManager>();
    });
    
    // ==========================================================================
    // 상태 관리자 생성자들 등록
    // ==========================================================================
    
    // 미사일류 (ALM, ASM, AAM)는 동일한 상태 관리자 사용
    RegisterStateManagerCreator(EN_WPN_KIND::WPN_KIND_ALM, []() -> std::unique_ptr<IWeaponStateManager> {
        return std::make_unique<MissileStateManager>(EN_WPN_KIND::WPN_KIND_ALM);
    });
    
    RegisterStateManagerCreator(EN_WPN_KIND::WPN_KIND_ASM, []() -> std::unique_ptr<IWeaponStateManager> {
        return std::make_unique<MissileStateManager>(EN_WPN_KIND::WPN_KIND_ASM);
    });
    
    RegisterStateManagerCreator(EN_WPN_KIND::WPN_KIND_AAM, []() -> std::unique_ptr<IWeaponStateManager> {
        return std::make_unique<MissileStateManager>(EN_WPN_KIND::WPN_KIND_AAM);
    });
    
    // 자항기뢰는 별도 상태 관리자 사용
    RegisterStateManagerCreator(EN_WPN_KIND::WPN_KIND_M_MINE, []() -> std::unique_ptr<IWeaponStateManager> {
        return std::make_unique<MineStateManager>();
    });
    
    // ==========================================================================
    // 무장 사양 정보 등록
    // ==========================================================================
    
    // 설정에서 값 읽기 (기본값 포함)
    double almMaxRange = config.GetDouble("Weapon.ALMMaxRange", 50.0);
    double asmMaxRange = config.GetDouble("Weapon.ASMMaxRange", 100.0);
    double aamMaxRange = config.GetDouble("Weapon.AAMMaxRange", 80.0);
    double mineMaxRange = config.GetDouble("Weapon.MineMaxRange", 30.0);
    
    double almSpeed = config.GetDouble("Weapon.ALMSpeed", 300.0);
    double asmSpeed = config.GetDouble("Weapon.ASMSpeed", 400.0);
    double aamSpeed = config.GetDouble("Weapon.AAMSpeed", 350.0);
    double mineSpeed = config.GetDouble("Weapon.MineSpeed", 5.0);
    
    double defaultLaunchDelay = config.GetDouble("Weapon.DefaultLaunchDelay", 3.0);
    
    // 무장 사양 등록
    m_weaponSpecs[EN_WPN_KIND::WPN_KIND_ALM] = WeaponSpecification(
        "ALM", almMaxRange, almSpeed, defaultLaunchDelay);
    
    m_weaponSpecs[EN_WPN_KIND::WPN_KIND_ASM] = WeaponSpecification(
        "ASM", asmMaxRange, asmSpeed, defaultLaunchDelay);
    
    m_weaponSpecs[EN_WPN_KIND::WPN_KIND_AAM] = WeaponSpecification(
        "AAM", aamMaxRange, aamSpeed, defaultLaunchDelay);
    
    m_weaponSpecs[EN_WPN_KIND::WPN_KIND_M_MINE] = WeaponSpecification(
        "MINE", mineMaxRange, mineSpeed, defaultLaunchDelay);
    
    std::cout << "Default weapon creators and specifications registered:" << std::endl;
    std::cout << "  - ALM: " << almMaxRange << "km, " << almSpeed << "m/s" << std::endl;
    std::cout << "  - ASM: " << asmMaxRange << "km, " << asmSpeed << "m/s" << std::endl;
    std::cout << "  - AAM: " << aamMaxRange << "km, " << aamSpeed << "m/s" << std::endl;
    std::cout << "  - MINE: " << mineMaxRange << "km, " << mineSpeed << "m/s" << std::endl;
}

} // namespace WeaponControl
