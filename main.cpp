#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>

//#include "Common/Utils/DebugPrint.h"
#include "Common/Communication/DdsComm.h"
#include "LaunchTubeManager.h"
#include "TubeMessageReceiver.h"

static std::unique_ptr<MINEASMALM::LaunchTubeManager> g_launchTubemanager = nullptr;
static std::unique_ptr<MINEASMALM::TubeMessageReceiver> g_messagereceiver = nullptr;
static bool g_running = true;

void signalHandler(int signal) {
    std::cout << "Signal " << signal << " received, shutting down..." << std::endl;
    g_running = false;
}
//int argc, char* argv[]
int main() {

    int tubeNumber = 1; //std::atoi(argv[1]);

    DebugLogger::Initialize(tubeNumber, "Hello");

    //if (argc != 2) {
    //    std::cerr << "Usage: LaunchTubeProcess.exe <tube_number>" << std::endl;
    //    std::cerr << "Example: LaunchTubeProcess.exe 1" << std::endl;
    //    return 1;
    //}


    if (tubeNumber < 1 || tubeNumber > 6) {
        std::cerr << "Tube number must be between 1 and 6" << std::endl;
        return 1;
    }

    // 신호 핸들러 등록
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    //DEBUG_STREAM(MAIN) << "Starting LaunchTubeProcess for Tube " << tubeNumber << std::endl;
#ifdef CONSOLMESSAGE
    std::cout << "Starting LaunchTubeProcess for Tube " << tubeNumber << std::endl;
#endif // CONSOLMESSAGE

    try {
        // DDS 통신 초기화
        auto ddsComm = std::make_shared<DdsComm>();
        if (!ddsComm->Initialize()) { // 송신용 메시지 등록
            DEBUG_ERROR_STREAM(MAIN) << "Failed to initialize DDS communication" << std::endl;
            //std::cerr << "Failed to initialize DDS communication" << std::endl;
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
    }
    catch (const std::exception& e) {
        DEBUG_ERROR_STREAM(MAIN) << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
