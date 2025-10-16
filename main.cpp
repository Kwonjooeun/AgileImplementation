
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>

#include "Common/Utils/DebugPrint.h"
#include "Common/Utils/ConfigManager.h"
#include "Common/Communication/DdsComm.h"
#include "LaunchTubeManager.h"
#include "TubeMessageReceiver.h"
#include <cassert>

//int argc, char* argv[]
int main() {
    static std::unique_ptr<AIEP::LaunchTubeManager> g_launchTubemanager = nullptr;
    static std::unique_ptr<AIEP::TubeMessageReceiver> g_messagereceiver = nullptr;
    static std::atomic<bool> g_running(true);

    // 1. 전체 설정 로드 (한 번만)
    auto& config = AIEP::ConfigManager::GetInstance();
    bool configLoaded = config.LoadFromFile("config.ini");
    assert(configLoaded);
    // 2. main에서는 시스템 인프라 설정만 관심
    const auto& sysInfra = config.GetSystemInfraConfig();

    // 디버깅용 임시 선언7
    int tubeNumber = 1;//std::atoi(argv[1]);

    DebugLogger::Initialize(tubeNumber);

    //if (argc != 2) {
    //    std::cerr << "Usage: LaunchTubeProcess.exe <tube_number>" << std::endl;
    //    std::cerr << "Example: LaunchTubeProcess.exe 1" << std::endl;
    //    return 1;
    //}

    if (tubeNumber < 1 || tubeNumber > sysInfra.totalTubes) {
#ifdef CONSOLMESSAGE
        std::cerr << "Tube number must be between 1 and 6" << std::endl;
#endif
        return 1;
    }


    DEBUG_STREAM(MAIN) << "Starting LaunchTubeProcess for Tube # " << tubeNumber << std::endl;
#ifdef CONSOLMESSAGE
    std::cout << "Starting LaunchTubeProcess for Tube " << tubeNumber << std::endl;
#endif // CONSOLMESSAGE

    try {
        // DDS 통신 초기화
        auto ddsComm = std::make_shared<AIEP::DdsComm>(sysInfra.ddsDomainId);

        // 발사관 생성 및 초기화
        g_launchTubemanager = std::make_unique<AIEP::LaunchTubeManager>(tubeNumber, ddsComm);

        // 메시지 핸들러 생성 및 초기화
        g_messagereceiver = std::make_unique<AIEP::TubeMessageReceiver>(tubeNumber, g_launchTubemanager.get(), ddsComm);

        ddsComm->Start();

        while (g_running.load())
        {
            // main 함수 종료 안되게 하는 loop
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        DEBUG_STREAM(MAIN) << "Shutting down systems..." << std::endl;
        
        ddsComm->Stop();
        
        // 정리 작업
        if (g_messagereceiver) {
            g_messagereceiver->Shutdown();
            g_messagereceiver.reset();
        }

        if (g_launchTubemanager) {
            g_launchTubemanager->Shutdown();
            g_launchTubemanager.reset();
        }

        DEBUG_STREAM(MAIN) << "Shutdown completed for Tube " << tubeNumber << std::endl;
        DebugLogger::Shutdown();
    }
    catch (const std::exception& e) {
        DEBUG_ERROR_STREAM(MAIN) << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
