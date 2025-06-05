#include "WeaponBase.h"
#include "../../Common/Utils/ConfigManager.h"
#include <iostream>

namespace WeaponControl {

// =============================================================================
// 생성자/소멸자
// =============================================================================

WeaponBase::WeaponBase(EN_WPN_KIND weaponKind)
    : m_weaponKind(weaponKind)
    , m_tubeNumber(0)
    , m_launched(false)
    , m_fireSolutionReady(false)
    , m_systemReady(false)
    , m_initialized(false)
    , m_specCached(false)
{
    m_initTime = std::chrono::steady_clock::now();
    m_lastUpdateTime = m_initTime;
    
    std::cout << "WeaponBase created for " << WeaponKindToString(weaponKind) << std::endl;
}

// =============================================================================
// IWeapon 인터페이스 구현
// =============================================================================

WeaponSpec WeaponBase::GetSpecification() const {
    std::lock_guard<std::mutex> lock(m_specMutex);
    
    if (!m_specCached) {
        m_cachedSpec = CreateSpecification();
        m_specCached = true;
    }
    
    return m_cachedSpec;
}

bool WeaponBase::Initialize(int tubeNumber) {
    std::lock_guard<std::mutex> lock(m_initMutex);
    
    if (m_initialized) {
        std::cout << "Weapon " << WeaponKindToString(m_weaponKind) 
                  << " already initialized" << std::endl;
        return true;
    }
    
    try {
        m_tubeNumber = tubeNumber;
        
        // 상태 초기화
        m_launched.store(false);
        m_fireSolutionReady.store(false);
        m_systemReady.store(false);
        
        // 시간 정보 초기화
        m_initTime = std::chrono::steady_clock::now();
        m_lastUpdateTime = m_initTime;
        
        // 파생 클래스 초기화
        if (!OnInitialize()) {
            std::cerr << "Weapon " << WeaponKindToString(m_weaponKind) 
                      << " specific initialization failed" << std::endl;
            return false;
        }
        
        // 시스템 준비 상태 초기 확인
        bool systemReady = CheckSystemReady();
        m_systemReady.store(systemReady);
        
        m_initialized = true;
        
        std::cout << "Weapon " << WeaponKindToString(m_weaponKind) 
                  << " initialized on tube " << tubeNumber 
                  << " (system ready: " << (systemReady ? "YES" : "NO") << ")" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during weapon initialization: " << e.what() << std::endl;
        return false;
    }
}

void WeaponBase::Reset() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    
    std::cout << "Resetting weapon " << WeaponKindToString(m_weaponKind) << std::endl;
    
    // 상태 리셋
    m_launched.store(false);
    m_fireSolutionReady.store(false);
    m_systemReady.store(false);
    
    // 콜백 제거
    {
        std::lock_guard<std::mutex> callbackLock(m_callbackMutex);
        m_launchStatusCallback = nullptr;
        m_systemReadyCallback = nullptr;
    }
    
    // 파생 클래스 리셋
    OnReset();
    
    // 캐시 무효화
    {
        std::lock_guard<std::mutex> specLock(m_specMutex);
        m_specCached = false;
    }
    
    std::cout << "Weapon " << WeaponKindToString(m_weaponKind) << " reset completed" << std::endl;
}

void WeaponBase::Update() {
    if (!m_initialized) {
        return;
    }
    
    m_lastUpdateTime = std::chrono::steady_clock::now();
    
    try {
        // 시스템 준비 상태 업데이트
        bool currentSystemReady = CheckSystemReady();
        bool previousSystemReady = m_systemReady.exchange(currentSystemReady);
        
        // 시스템 준비 상태 변경 알림
        if (currentSystemReady != previousSystemReady) {
            NotifySystemReadyChanged(currentSystemReady);
        }
        
        // 파생 클래스 업데이트
        OnUpdate();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during weapon update: " << e.what() << std::endl;
    }
}

void WeaponBase::SetLaunched(bool launched) {
    bool oldValue = m_launched.exchange(launched);
    
    if (oldValue != launched) {
        std::cout << "Weapon " << WeaponKindToString(m_weaponKind) 
                  << " launch status changed: " << (launched ? "LAUNCHED" : "NOT_LAUNCHED") << std::endl;
        
        NotifyLaunchStatusChanged(launched);
    }
}

void WeaponBase::SetFireSolutionReady(bool ready) {
    bool oldValue = m_fireSolutionReady.exchange(ready);
    
    if (oldValue != ready) {
        std::cout << "Weapon " << WeaponKindToString(m_weaponKind) 
                  << " fire solution ready: " << (ready ? "YES" : "NO") << std::endl;
    }
}

bool WeaponBase::IsSystemReady() const {
    return m_systemReady.load();
}

void WeaponBase::SetLaunchStatusCallback(std::function<void(bool)> callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_launchStatusCallback = callback;
}

void WeaponBase::SetSystemReadyCallback(std::function<void(bool)> callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_systemReadyCallback = callback;
}

// =============================================================================
// 콜백 알림 헬퍼 함수들
// =============================================================================

void WeaponBase::NotifyLaunchStatusChanged(bool launched) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    
    if (m_launchStatusCallback) {
        try {
            m_launchStatusCallback(launched);
        } catch (const std::exception& e) {
            std::cerr << "Exception in launch status callback: " << e.what() << std::endl;
        }
    }
}

void WeaponBase::NotifySystemReadyChanged(bool ready) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    
    if (m_systemReadyCallback) {
        try {
            m_systemReadyCallback(ready);
        } catch (const std::exception& e) {
            std::cerr << "Exception in system ready callback: " << e.what() << std::endl;
        }
    }
}

} // namespace WeaponControl
