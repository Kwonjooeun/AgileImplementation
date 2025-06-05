#pragma once

#include <variant>
#include <string>
#include <memory>
#include <atomic>
#include <chrono>
#include <exception>
#include <thread>

// DDS 메시지 타입들 포함
#include "../../dds_message/AIEP_AIEP_.hpp"

namespace WeaponControl {

// =============================================================================
// Result 타입 - 일관된 에러 처리
// =============================================================================

struct ErrorInfo {
    std::string message;
    int code;
    
    ErrorInfo(const std::string& msg = "", int c = -1) 
        : message(msg), code(c) {}
};

template<typename T = void>
class Result {
private:
    std::variant<T, ErrorInfo> m_data;
    
    explicit Result(T&& value) : m_data(std::forward<T>(value)) {}
    explicit Result(const ErrorInfo& error) : m_data(error) {}
    
public:
    static Result<T> success(T&& value) { 
        return Result(std::forward<T>(value)); 
    }
    
    static Result<T> failure(const std::string& message, int code = -1) { 
        return Result(ErrorInfo{message, code}); 
    }
    
    bool isSuccess() const { return std::holds_alternative<T>(m_data); }
    bool isFailure() const { return !isSuccess(); }
    
    const T& value() const { return std::get<T>(m_data); }
    T& value() { return std::get<T>(m_data); }
    
    const ErrorInfo& error() const { return std::get<ErrorInfo>(m_data); }
    
    // 편의 연산자
    explicit operator bool() const { return isSuccess(); }
};

// void 특화
template<>
class Result<void> {
private:
    std::optional<ErrorInfo> m_error;
    
    explicit Result(const ErrorInfo& error) : m_error(error) {}
    Result() : m_error(std::nullopt) {}
    
public:
    static Result<void> success() { return Result(); }
    
    static Result<void> failure(const std::string& message, int code = -1) { 
        return Result(ErrorInfo{message, code}); 
    }
    
    bool isSuccess() const { return !m_error.has_value(); }
    bool isFailure() const { return m_error.has_value(); }
    
    const ErrorInfo& error() const { return m_error.value(); }
    
    explicit operator bool() const { return isSuccess(); }
};

// =============================================================================
// Cancellation Token - ABORT 우선순위 처리
// =============================================================================

class OperationCancelledException : public std::exception {
public:
    const char* what() const noexcept override {
        return "Operation was cancelled";
    }
};

class CancellationToken {
private:
    std::shared_ptr<std::atomic<bool>> m_cancelled;
    
public:
    CancellationToken() : m_cancelled(std::make_shared<std::atomic<bool>>(false)) {}
    
    void cancel() { m_cancelled->store(true); }
    bool isCancelled() const { return m_cancelled->load(); }
    
    void throwIfCancelled() const {
        if (isCancelled()) {
            throw OperationCancelledException();
        }
    }
    
    // 지정된 시간 동안 대기하면서 취소 확인
    template<typename Rep, typename Period>
    bool waitFor(const std::chrono::duration<Rep, Period>& duration) const {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < duration) {
            if (isCancelled()) {
                return false; // 취소됨
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true; // 정상 완료
    }
};

// =============================================================================
// 시스템 통계
// =============================================================================

struct SystemStatistics {
    uint32_t totalCommands;
    uint32_t successfulCommands;
    uint32_t failedCommands;
    std::chrono::steady_clock::time_point systemStartTime;
    std::chrono::steady_clock::time_point lastUpdateTime;
    
    SystemStatistics()
        : totalCommands(0), successfulCommands(0), failedCommands(0)
        , systemStartTime(std::chrono::steady_clock::now())
        , lastUpdateTime(std::chrono::steady_clock::now()) {}
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

/**
 * @brief 설정 명령을 문자열로 변환
 */
inline std::string SetCmdToString(EN_SET_CMD cmd) {
    switch(cmd) {
        case EN_SET_CMD::SET_CMD_SET: return "SET";
        case EN_SET_CMD::SET_CMD_UNSET: return "UNSET";
        default: return "UNKNOWN";
    }
}

} // namespace WeaponControl
