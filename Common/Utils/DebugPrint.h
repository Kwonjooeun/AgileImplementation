// Common/Utils/DebugPrint.h
#pragma once
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <string>
#include <mutex>
#include <filesystem>
#include <sstream>

#ifdef _DEBUG
#define DEBUG_ENABLED 1
#else
#define DEBUG_ENABLED 0
#endif

// 모듈별 디버그 제어 (기존과 동일)
#if DEBUG_ENABLED
#define ENABLE_MAIN_DEBUG                    1
#define ENABLE_LAUNCHTUBEMANAGER_DEBUG       1
#define ENABLE_MESSAGERECEIVER_DEBUG         1
#define ENABLE_WEAPONFACTORY_DEBUG           0
#define ENABLE_WEAPONSTATE_DEBUG             1
#define ENABLE_ENGAGEMENT_DEBUG              1
#define ENABLE_DDSCOMM_DEBUG                 1
#define ENABLE_MINEMANAGER_DEBUG             0
#else
#define ENABLE_MAIN_DEBUG           0
#define ENABLE_LAUNCHTUBE_DEBUG     0
#define ENABLE_MESSAGEHANDLER_DEBUG 0
#define ENABLE_WEAPONFACTORY_DEBUG  0
#define ENABLE_WEAPONSTATE_DEBUG    0
#define ENABLE_ENGAGEMENT_DEBUG     0
#define ENABLE_DDSCOMM_DEBUG        0
#define ENABLE_MINEMANAGER_DEBUG    0
#endif

class DebugLogger {
private:
    static std::ofstream logFile;
    static std::mutex logMutex;
    static bool initialized;
    static int tubeNumber;  // 발사관 번호 저장

public:
    // 발사관 번호를 포함한 초기화 함수
    static void Initialize(int tubeNum, const std::string& customLogFileName = "") {
        std::lock_guard<std::mutex> lock(logMutex);
        if (initialized) return;

        tubeNumber = tubeNum;
        std::string fileName;

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        if (!customLogFileName.empty()) {
            fileName = customLogFileName;
        }
        else {
            // 자동 파일명 생성: debug_tube{N}_YYYYMMDD_HHMMSS.log
            char buffer[100];
            std::snprintf(buffer, sizeof(buffer), "debug_tube%d_%04d%02d%02d_%02d%02d%02d.log",
                tubeNum,
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
            fileName = buffer;
        }

        // logs 디렉토리 생성
        std::filesystem::create_directories("logs");
        std::string fullPath = "logs/" + fileName;
        logFile.open(fullPath, std::ios::app);
        if (logFile.is_open()) {
            logFile << "\n========== Tube " << tubeNum << " Debug Session Started ==========" << std::endl;
            logFile << "Process started at: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << std::endl;
            logFile << "==========================================================" << std::endl;
            initialized = true;
            std::cout << "Debug log file created for Tube " << tubeNum << ": " << fullPath << std::endl;
        }
        else {
            std::cerr << "Failed to create log file: " << fullPath << std::endl;
        }
    }

    // 기존 Initialize 함수 (하위 호환성)
    static void Initialize(const std::string& logFileName = "") {
        Initialize(0, logFileName); // 기본값으로 0번 사용
    }

    static void Shutdown() {
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);

            logFile << "==========================================================" << std::endl;
            logFile << "Process ended at: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << std::endl;
            logFile << "========== Tube " << tubeNumber << " Debug Session Ended ==========\n" << std::endl;
            logFile.close();
        }
        initialized = false;
    }

    static void WriteToFile(const std::string& level, const std::string& module, const std::string& message) {
        if (!initialized) return;

        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

            logFile << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
                << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
                << "[TUBE" << tubeNumber << "] "
                << "[" << level << "] "
                << "[" << module << "] " << message << std::endl;
            logFile.flush();
        }
    }

    static int GetTubeNumber() { return tubeNumber; }
};

// 콘솔 + 파일 동시 출력을 위한 커스텀 스트림 클래스 (기존과 동일)
class DualOutputStream {
private:
    std::string module;
    std::string level;
    std::ostringstream buffer;

public:
    DualOutputStream(const std::string& mod, const std::string& lvl) : module(mod), level(lvl) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        if (level == "ERROR") {
            std::cerr << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
                << "[TUBE" << DebugLogger::GetTubeNumber() << "] "
                << "[" << module << "] [ERROR] ";
        }
        else if (level == "WARNING") {
            std::cerr << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
                << "[TUBE" << DebugLogger::GetTubeNumber() << "] "
                << "[" << module << "] [WARNING] ";
        }
        else {
            std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
                << "[TUBE" << DebugLogger::GetTubeNumber() << "] "
                << "[" << module << "] ";
        }
    }

    ~DualOutputStream() {
        std::string content = buffer.str();
        if (!content.empty()) {
            DebugLogger::WriteToFile(level, module, content);
        }
    }

    template<typename T>
    DualOutputStream& operator<<(const T& value) {
        if (level == "ERROR") {
            std::cerr << value;
        }
        else {
            std::cout << value;
        }

        buffer << value;
        return *this;
    }

    DualOutputStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        if (level == "ERROR") {
            std::cerr << manip;
        }
        else {
            std::cout << manip;
        }

        buffer << manip;
        return *this;
    }
};

// 기존 매크로 (변경 없음)
#define DEBUG_STREAM(module) \
    if (ENABLE_##module##_DEBUG) \
        DualOutputStream(#module, "DEBUG")

#define DEBUG_ERROR_STREAM(module) \
    if (ENABLE_##module##_DEBUG) \
        DualOutputStream(#module, "ERROR")

#define DEBUG_WARNING_STREAM(module) \
    if (ENABLE_##module##_DEBUG) \
        DualOutputStream(#module, "WARNING")
