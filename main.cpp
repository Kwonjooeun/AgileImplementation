#include "LaunchTube.h"
#include "TubeMessageHandler.h"
#include "Common/Communication/DdsComm.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>

static std::unique_ptr<LaunchTube> g_launchTube = nullptr;
static std::unique_ptr<TubeMessageHandler> g_messageHandler = nullptr;
static bool g_running = true;

void signalHandler(int signal) {
    std::cout << "Signal " << signal << " received, shutting down..." << std::endl;
    g_running = false;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: LaunchTubeProcess.exe <tube_number>" << std::endl;
        std::cerr << "Example: LaunchTubeProcess.exe 1" << std::endl;
        return 1;
    }

    int tubeNumber = std::atoi(argv[1]);
    if (tubeNumber < 1 || tubeNumber > 6) {
        std::cerr << "Tube number must be between 1 and 6" << std::endl;
        return 1;
    }

    // 신호 핸들러 등록
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    DEBUG_STREAM(MAIN) << "Starting LaunchTubeProcess for Tube " << tubeNumber << std::endl;
    
    try {
        // DDS 통신 초기화
        auto ddsComm = std::make_shared<DdsComm>();
        if (!ddsComm->Initialize()) {
            DEBUG_ERROR_STREAM(MAIN) << "Failed to initialize DDS communication" << std::endl;
            return 1;
        }


        // 발사관 생성 및 초기화
        g_launchTube = std::make_unique<LaunchTube>(tubeNumber);
        if (!g_launchTube->Initialize()) {
            DEBUG_ERROR_STREAM(MAIN) << "Failed to initialize LaunchTube " << tubeNumber << std::endl;
            return 1;
        }

        // 메시지 핸들러 생성 및 초기화
        g_messageHandler = std::make_unique<TubeMessageHandler>(tubeNumber, g_launchTube.get(), ddsComm);
        if (!g_messageHandler->Initialize()) {
            DEBUG_ERROR_STREAM(MAIN) << "Failed to initialize TubeMessageHandler" << std::endl;
            return 1;
        }

        ddsComm->Start();

        DEBUG_STREAM(MAIN) << "LaunchTubeProcess " << tubeNumber << " started successfully" << std::endl;

        // 메인 루프
        while (g_running) {
            try {
                // 발사관 주기적 업데이트
                g_launchTube->Update();

                // 메시지 핸들러 업데이트
                g_messageHandler->Update();

                // 100ms 대기
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            catch (const std::exception& e) {
                std::cerr << "Error in main loop: " << e.what() << std::endl;
            }
        }

        DEBUG_ERROR_STREAM(MAIN) << "Error in main loop: " << e.what() << std::endl;
        
        // 정리
        if (g_messageHandler) {
            g_messageHandler->Shutdown();
            g_messageHandler.reset();
        }

        if (g_launchTube) {
            g_launchTube->Shutdown();
            g_launchTube.reset();
        }

        ddsComm->Stop();

    }
    catch (const std::exception& e) {
        DEBUG_ERROR_STREAM(MAIN) << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    DEBUG_STREAM(MAIN) << "LaunchTubeProcess " << tubeNumber << " terminated" << std::endl;
    return 0;
}
