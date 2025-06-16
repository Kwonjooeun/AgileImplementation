#include "DebugPrint.h"

// Static 멤버 정의
std::ofstream DebugLogger::logFile;
std::mutex DebugLogger::logMutex;
bool DebugLogger::initialized = false;
int DebugLogger::tubeNumber = 0;

