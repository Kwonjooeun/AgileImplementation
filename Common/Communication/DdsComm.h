#pragma once

#include "../dds_message/AIEP_AIEP_.hpp"
#include "../dds_library/dds.h"
#include <functional>
#include <map>
#include <iostream>

// =============================================================================
// RTI DDS 통신 관리 클래스
// =============================================================================

class DdsComm {
public:
    DdsComm();
    ~DdsComm();

    // 시작/정지
    bool Initialize();
    inline void Start() { dds.Start(); }
    inline void Stop() { dds.Stop(); }

    // 메시지 송신 (템플릿)
    template <typename T>
    void Send(const T& message)
    {
        dds.Send(message);
    }

    template<typename MessageType>
    void RegisterReader(std::function<void(const MessageType&)> callback) {
        dds.RegisterReader<MessageType>(callback);
        std::cout << "Reader registered for: " << typeid(MessageType).name() << std::endl;
    }

    // Writer 등록 (생성자에서 일괄 등록)
    void RegisterWriters();

private:
    Dds dds;
};
