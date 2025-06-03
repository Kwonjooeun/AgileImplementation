#include "DdsComm.h"

DdsComm::DdsComm() {
    Initialize();
}

bool DdsComm::Initialize()
{
    RegisterWriters();
    return 1;
}

void DdsComm::RegisterWriters() {
    // 송신용 Writer들 등록
    dds.RegisterWriter<AIEP_CMSHCI_M_MINE_ALL_PLAN_LIST>();
    dds.RegisterWriter<AIEP_M_MINE_EP_RESULT>();
    dds.RegisterWriter<AIEP_ALM_ASM_EP_RESULT>();
    dds.RegisterWriter<AIEP_ASSIGN_RESP>();
    dds.RegisterWriter<AIEP_AI_INFER_RESULT_WP>();
    dds.RegisterWriter<AIEP_INTERNAL_INFER_REQ>();
}
