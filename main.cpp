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

static std::unique_ptr<MINEASMALM::LaunchTubeManager> g_launchTubemanager = nullptr;
static std::unique_ptr<MINEASMALM::TubeMessageReceiver> g_messagereceiver = nullptr;
static std::atomic<bool> g_running(true);

//int argc, char* argv[]
int main() {
    // 1. 전체 설정 로드 (한 번만)
    auto& config = MINEASMALM::ConfigManager::GetInstance();
    if (!config.LoadFromFile("config.ini")) {
        return 1;
    }

    // 2. main에서는 시스템 인프라 설정만 관심
    const auto& sysInfra = config.GetSystemInfraConfig();

    // 디버깅용 임시 선언
    int tubeNumber = 1; //std::atoi(argv[1]);
    DebugLogger::Initialize(tubeNumber);

    if (tubeNumber < 1 || tubeNumber > sysInfra.totalTubes) {
#ifdef CONSOLMESSAGE
        std::cerr << "Tube number must be between 1 and 6" << std::endl;
#endif
        return 1;
    }

    // 신호 핸들러 등록
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    DEBUG_STREAM(MAIN) << "Starting LaunchTubeProcess for Tube # " << tubeNumber << std::endl;
#ifdef CONSOLMESSAGE
    std::cout << "Starting LaunchTubeProcess for Tube " << tubeNumber << std::endl;
#endif // CONSOLMESSAGE

    try {
        // DDS 통신 초기화
        auto ddsComm = std::make_shared<DdsComm>();
        if (!ddsComm->Initialize()) { // 송신용 메시지 등록
            DEBUG_ERROR_STREAM(MAIN) << "Failed to initialize DDS communication" << std::endl;
            return 1;
        }

        // 발사관 생성 및 초기화
        g_launchTubemanager = std::make_unique<MINEASMALM::LaunchTubeManager>(tubeNumber);
        if (!g_launchTubemanager->Initialize()) {
            DEBUG_ERROR_STREAM(MAIN) << "Failed to initialize LaunchTubeManager " << tubeNumber << std::endl;
            return 1;
        }

        // 메시지 핸들러 생성 및 초기화
        g_messagereceiver = std::make_unique<MINEASMALM::TubeMessageReceiver>(tubeNumber, g_launchTubemanager.get(), ddsComm);
        if (!g_messagereceiver->Initialize()) {
            DEBUG_ERROR_STREAM(MAIN) << "Failed to initialize TubeMessageReceiver" << std::endl;
            return 1;
        }

        ddsComm->Start();

        while (g_running.load())
        {
            // main 함수 종료 안되게 하는 loop
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
                DEBUG_STREAM(MAIN) << "Shutting down systems..." << std::endl;

        // 정리 작업
        if (g_messagereceiver) {
            g_messagereceiver->Shutdown();
            g_messagereceiver.reset();
        }

        if (g_launchTubemanager) {
            g_launchTubemanager->Shutdown();
            g_launchTubemanager.reset();
        }

        ddsComm->Stop();
        DEBUG_STREAM(MAIN) << "Shutdown completed for Tube " << tubeNumber << std::endl;
        DebugLogger::Shutdown();
    }
    catch (const std::exception& e) {
        DEBUG_ERROR_STREAM(MAIN) << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
