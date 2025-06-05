#include "WeaponStateManagerBase.h"
#include "../../Common/Utils/ConfigManager.h"
#include <iostream>
#include <algorithm>

namespace WeaponControl {

// =============================================================================
// 기본 상태 전이 맵
// =============================================================================
const std::map<EN_WPN_CTRL_STATE, std::set<EN_WPN_CTRL_STATE>> WeaponStateManagerBase::s_defaultTransitionMap = {
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON}},
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_RTL, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH, EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT}},
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}},
    {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POST_LAUNCH, {EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF}}
};

// =============================================================================
// 생성자/소멸자
// =============================================================================

WeaponStateManagerBase::WeaponStateManagerBase(EN_WPN_KIND weaponKind)
    : m_weaponKind(weaponKind)
    , m_tubeNumber(0)
    , m_currentState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF)
    , m_interlockStatus(false)
    , m_engagementPlanReady(false)
    , m_cancelRequested(false)
    , m_launchInProgress(false)
    , m_initialized(false)
{
    m_stateStartTime = std::chrono::steady_clock::now();
    
    std::cout << "WeaponStateManagerBase created for " << WeaponKindToString(weaponKind) << std::endl;
}

WeaponStateManagerBase::~WeaponStateManagerBase() {
    // 진행 중인 발사 절차 중단
    CancelCurrentOperation();
    
    if (m_launchThread && m_launchThread->joinable()) {
        m_launchThread->join();
    }
}

// =============================================================================
// IWeaponStateManager 인터페이스 구현
// =============================================================================

bool WeaponStateManagerBase::Initialize(int tubeNumber, EN_WPN_KIND weaponKind) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    if (m_initialized) {
        std::cout << "WeaponStateManager already initialized" << std::endl;
        return true;
    }
    
    try {
        m_tubeNumber = tubeNumber;
        m_weaponKind = weaponKind;
        
        // 상태 초기화
        m_currentState.store(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
        m_stateStartTime = std::chrono::steady_clock::now();
        
        m_interlockStatus.store(false);
        m_engagementPlanReady.store(false);
        m_cancelRequested.store(false);
        m_launchInProgress.store(false);
        
        // 파생 클래스 초기화
        if (!OnInitialize()) {
            std::cerr << "Weapon-specific state manager initialization failed" << std::endl;
            return false;
        }
        
        m_initialized = true;
        
        std::cout << "WeaponStateManager initialized for " << WeaponKindToString(weaponKind) 
                  << " on tube " << tubeNumber << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during state manager initialization: " << e.what() << std::endl;
        return false;
    }
}

void WeaponStateManagerBase::SetWeapon(std::weak_ptr<IWeapon> weapon) {
    std::lock_guard<std::mutex> lock(m_weaponMutex);
    m_weapon = weapon;
    
    std::cout << "Weapon object set for state manager" << std::endl;
}

void WeaponStateManagerBase::Reset() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    std::cout << "Resetting WeaponStateManager" << std::endl;
    
    // 진행 중인 작업 취소
    CancelCurrentOperation();
    
    // 상태 초기화
    EN_WPN_CTRL_STATE oldState = m_currentState.load();
    m_currentState.store(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
    m_stateStartTime = std::chrono::steady_clock::now();
    
    m_interlockStatus.store(false);
    m_engagementPlanReady.store(false);
    
    // 콜백 알림
    if (oldState != EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF) {
        NotifyStateChanged(oldState, EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
    }
    
    std::cout << "WeaponStateManager reset completed" << std::endl;
}

EN_WPN_CTRL_STATE WeaponStateManagerBase::GetCurrentState() const {
    return m_currentState.load();
}

bool WeaponStateManagerBase::RequestStateChange(EN_WPN_CTRL_STATE newState) {
    EN_WPN_CTRL_STATE currentState = m_currentState.load();
    
    // ABORT 명령은 언제든지 허용
    if (newState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT) {
        return RequestAbort();
    }
    
    // 유효한 전이인지 확인
    if (!IsValidTransition(currentState, newState)) {
        std::cerr << "Invalid state transition from " << StateToString(currentState) 
                  << " to " << StateToString(newState) << std::endl;
        return false;
    }
    
    std::cout << "Requesting state change: " << StateToString(currentState) 
              << " -> " << StateToString(newState) << std::endl;
    
    try {
        bool result = false;
        
        switch (newState) {
            case EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF:
                result = ProcessTurnOff();
                break;
            case EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON:
                result = ProcessTurnOn();
                break;
            case EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH:
                result = ProcessLaunch();
                break;
            default:
                // 직접 상태 변경
                SetState(newState);
                result = true;
                break;
        }
        
        if (result) {
            std::cout << "State change successful: " << StateToString(currentState) 
                      << " -> " << StateToString(newState) << std::endl;
        } else {
            std::cerr << "State change failed: " << StateToString(currentState) 
                      << " -> " << StateToString(newState) << std::endl;
        }
        
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during state change: " << e.what() << std::endl;
        return false;
    }
}

bool WeaponStateManagerBase::IsValidTransition(EN_WPN_CTRL_STATE from, EN_WPN_CTRL_STATE to) const {
    auto transitionMap = GetValidTransitionMap();
    auto it = transitionMap.find(from);
    return it != transitionMap.end() && it->second.count(to) > 0;
}

bool WeaponStateManagerBase::RequestAbort() {
    std::cout << "ABORT requested" << std::endl;
    
    CancelCurrentOperation();
    
    EN_WPN_CTRL_STATE oldState = m_currentState.load();
    SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT);
    
    return ProcessAbortInternal();
}

bool WeaponStateManagerBase::CheckInterlockConditions() const {
    // 기본 인터록 조건들
    bool baseInterlocks = m_interlockStatus.load() && m_engagementPlanReady.load();
    
    // 무장별 추가 인터록 조건
    bool weaponSpecificInterlocks = CheckWeaponSpecificInterlocks();
    
    return baseInterlocks && weaponSpecificInterlocks;
}

void WeaponStateManagerBase::SetInterlockStatus(bool status) {
    bool oldStatus = m_interlockStatus.exchange(status);
    
    if (oldStatus != status) {
        std::cout << "Interlock status changed: " << (status ? "OK" : "NOT_OK") << std::endl;
    }
}

void WeaponStateManagerBase::SetEngagementPlanReady(bool ready) {
    bool oldReady = m_engagementPlanReady.exchange(ready);
    
    if (oldReady != ready) {
        std::cout << "Engagement plan ready: " << (ready ? "YES" : "NO") << std::endl;
    }
}

void WeaponStateManagerBase::Update() {
    if (!m_initialized) {
        return;
    }
    
    EN_WPN_CTRL_STATE currentState = m_currentState.load();
    
    // RTL 상태 자동 전이 확인
    if (currentState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON) {
        if (CheckInterlockConditions()) {
            std::cout << "Conditions met, transitioning to RTL" << std::endl;
            SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_RTL);
        }
    }
    else if (currentState == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_RTL) {
        if (!CheckInterlockConditions()) {
            std::cout << "Conditions not met, returning to ON" << std::endl;
            SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON);
        }
    }
    
    // 상태별 주기적 처리
    OnStateUpdate(currentState);
}

void WeaponStateManagerBase::SetStateChangeCallback(std::function<void(EN_WPN_CTRL_STATE, EN_WPN_CTRL_STATE)> callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_stateChangeCallback = callback;
}

void WeaponStateManagerBase::SetLaunchStatusCallback(std::function<void(bool)> callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_launchStatusCallback = callback;
}

bool WeaponStateManagerBase::IsReadyToLaunch() const {
    return m_currentState.load() == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_RTL;
}

bool WeaponStateManagerBase::IsLaunching() const {
    return m_currentState.load() == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH;
}

bool WeaponStateManagerBase::IsPostLaunch() const {
    return m_currentState.load() == EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POST_LAUNCH;
}

std::chrono::steady_clock::time_point WeaponStateManagerBase::GetStateStartTime() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_stateStartTime;
}

std::chrono::milliseconds WeaponStateManagerBase::GetStateDuration() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_stateStartTime);
}

// =============================================================================
// Protected 메서드들
// =============================================================================

std::map<EN_WPN_CTRL_STATE, std::set<EN_WPN_CTRL_STATE>> WeaponStateManagerBase::GetValidTransitionMap() const {
    return s_defaultTransitionMap;
}

std::vector<LaunchStep> WeaponStateManagerBase::GetLaunchSteps() const {
    // 기본 발사 단계들
    return {
        {"Power On Check", 1.0f}, 
        {"System Verification", 1.0f}, 
        {"Launch Sequence", 1.0f}
    };
}

float WeaponStateManagerBase::GetPowerOnCheckDuration() const {
    auto& config = ConfigManager::GetInstance();
    return config.GetFloat("Weapon.DefaultLaunchDelay", 3.0f);
}

void WeaponStateManagerBase::SetState(EN_WPN_CTRL_STATE newState) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    EN_WPN_CTRL_STATE oldState = m_currentState.exchange(newState);
    m_stateStartTime = std::chrono::steady_clock::now();
    
    if (oldState != newState) {
        OnStateExit(oldState);
        OnStateEnter(newState);
        NotifyStateChanged(oldState, newState);
    }
}

bool WeaponStateManagerBase::SleepWithCancellationCheck(float duration) {
    const int interval_ms = 50;
    int total_intervals = static_cast<int>(duration * 1000 / interval_ms);
    
    for (int i = 0; i < total_intervals; ++i) {
        if (m_cancelRequested.load()) {
            std::cout << "Operation cancelled" << std::endl;
            return false;
        }
        
        std::unique_lock<std::mutex> lock(m_cancelMutex);
        m_cancelCondition.wait_for(lock, std::chrono::milliseconds(interval_ms));
        
        if (m_cancelRequested.load()) {
            return false;
        }
    }
    
    return true;
}

std::shared_ptr<IWeapon> WeaponStateManagerBase::GetWeapon() const {
    std::lock_guard<std::mutex> lock(m_weaponMutex);
    return m_weapon.lock();
}

void WeaponStateManagerBase::NotifyStateChanged(EN_WPN_CTRL_STATE oldState, EN_WPN_CTRL_STATE newState) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    
    if (m_stateChangeCallback) {
        try {
            m_stateChangeCallback(oldState, newState);
        } catch (const std::exception& e) {
            std::cerr << "Exception in state change callback: " << e.what() << std::endl;
        }
    }
}

void WeaponStateManagerBase::NotifyLaunchStatusChanged(bool launched) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    
    if (m_launchStatusCallback) {
        try {
            m_launchStatusCallback(launched);
        } catch (const std::exception& e) {
            std::cerr << "Exception in launch status callback: " << e.what() << std::endl;
        }
    }
}

// =============================================================================
// Private 메서드들 - 상태 전이 처리
// =============================================================================

bool WeaponStateManagerBase::ProcessTurnOn() {
    EN_WPN_CTRL_STATE oldState = m_currentState.load();
    OnStateExit(oldState);
    
    SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POC);
    OnStateEnter(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POC);
    
    std::cout << "Performing power-on check..." << std::endl;
    
    float pocDuration = GetPowerOnCheckDuration();
    if (!SleepWithCancellationCheck(pocDuration)) {
        SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
        return false;
    }
    
    OnStateExit(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POC);
    SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON);
    OnStateEnter(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ON);
    
    std::cout << "Power-on check complete" << std::endl;
    return true;
}

bool WeaponStateManagerBase::ProcessTurnOff() {
    CancelCurrentOperation();
    
    EN_WPN_CTRL_STATE oldState = m_currentState.load();
    OnStateExit(oldState);
    SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
    OnStateEnter(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_OFF);
    
    std::cout << "Weapon turned off" << std::endl;
    return true;
}

bool WeaponStateManagerBase::ProcessLaunch() {
    if (m_launchInProgress.load()) {
        std::cerr << "Launch already in progress" << std::endl;
        return false;
    }
    
    // 발사 절차를 별도 스레드에서 실행
    if (m_launchThread && m_launchThread->joinable()) {
        m_launchThread->join();
    }
    
    m_launchThread = std::make_unique<std::thread>(&WeaponStateManagerBase::LaunchSequenceThread, this);
    
    return true;
}

bool WeaponStateManagerBase::ProcessAbortInternal() {
    std::cout << "Processing abort command" << std::endl;
    
    // 발사 절차 중단
    if (m_launchThread && m_launchThread->joinable()) {
        m_launchThread->join();
    }
    
    m_launchInProgress.store(false);
    
    return true;
}

// =============================================================================
// 스레드 관련 함수들
// =============================================================================

void WeaponStateManagerBase::LaunchSequenceThread() {
    m_launchInProgress.store(true);
    m_cancelRequested.store(false);
    
    EN_WPN_CTRL_STATE oldState = m_currentState.load();
    OnStateExit(oldState);
    SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH);
    OnStateEnter(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH);
    
    std::cout << "Starting launch sequence..." << std::endl;
    
    auto launchSteps = GetLaunchSteps();
    bool success = true;
    
    for (const auto& step : launchSteps) {
        if (m_cancelRequested.load()) {
            std::cout << "Launch sequence aborted" << std::endl;
            SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT);
            success = false;
            break;
        }
        
        std::cout << "Launch step: " << step.description 
                  << " (Duration: " << step.duration_sec << " seconds)" << std::endl;
        
        if (!SleepWithCancellationCheck(step.duration_sec)) {
            std::cout << "Launch sequence cancelled" << std::endl;
            SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_ABORT);
            success = false;
            break;
        }
    }
    
    if (success) {
        // 무장에 발사 상태 알림
        auto weapon = GetWeapon();
        if (weapon) {
            weapon->SetLaunched(true);
        }
        
        OnStateExit(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_LAUNCH);
        SetState(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POST_LAUNCH);
        OnStateEnter(EN_WPN_CTRL_STATE::WPN_CTRL_STATE_POST_LAUNCH);
        
        NotifyLaunchStatusChanged(true);
        
        std::cout << "Launch sequence completed successfully" << std::endl;
    }
    
    m_launchInProgress.store(false);
}

void WeaponStateManagerBase::CancelCurrentOperation() {
    if (m_cancelRequested.exchange(true)) {
        return; // 이미 취소 요청됨
    }
    
    std::cout << "Cancelling current operation..." << std::endl;
    
    // 대기 중인 스레드들 깨우기
    {
        std::lock_guard<std::mutex> lock(m_cancelMutex);
        m_cancelCondition.notify_all();
    }
    
    // 발사 스레드 종료 대기
    if (m_launchThread && m_launchThread->joinable()) {
        m_launchThread->join();
    }
    
    std::cout << "Operation cancelled" << std::endl;
}

} // namespace WeaponControl
