#pragma once

#include "IEngagementManager.h"
#include "../../Common/Utils/ConfigManager.h"
#include "../../Common/Utils/DebugPrint.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

namespace AIEP {

    // =============================================================================
    // 기본 교전계획 관리자 (공통 기능)
    // =============================================================================
    class EngagementManagerBase : public IEngagementManager {
    public:
        EngagementManagerBase(ST_WA_SESSION weaponAssignInfo, std::shared_ptr<AIEP::DdsComm> ddsComm);
        virtual ~EngagementManagerBase();

        //bool Initialize(int tubeNumber, EN_WPN_KIND weaponKind) override;
        void Reset() override;
        void Shutdown() override;
        void StartEngagementPlanManager() override;
        void WorkerLoop() override;

        // DDS 메시지 처리 (LaunchTubeManager에서 직접 호출)
        void UpdateOwnShipInfo(const NAVINF_SHIP_NAVIGATION_INFO& ownShip) override;
        void UpdateSystemTargetInfo(const TRKMGR_SYSTEMTARGET_INFO& target) override;
        void UpdatePAInfo(const CMSHCI_AIEP_PA_INFO& paInfo) override;
        void UpdateWaypoints(const CMSHCI_AIEP_WPN_GEO_WAYPOINTS& waypoints) override;
        bool UpdateWeaponAssignmentInformation(const ST_WA_SESSION weaponAssignInfo) override;
        void RequestAIWaypointInference(const CMSHCI_AIEP_AI_WAYPOINTS_INFERENCE_REQ& AIWPInferReq) override;
        void ProcessAIInferredWaypoints(const AIEP_INTERNAL_INFER_RESULT_WP& AIWPInferReq) override;

        // 발사 완료 여부 확인 (WpnStatusCtrlManager 에서 호출하는 callback 함수)
        void WeaponLaunched(std::chrono::steady_clock::time_point launchTime) override;

        // 교전계획 준비 상태 확인
        bool IsEngagementPlanReady() const override {
            return m_engagementPlanReady.load();
        }

        //template <typename T> //검토: 의존관계를 단순히 하기 위해 여기에 추가하거나, Weapon Control system에서만 Dds관련 클래스를 사용하는건 어떨지
        //void SendMessage(const T& message)
        //{
        //    m_ddsComm->Send(message);
        //}

    protected:
        void WeaponSpecInitialization() override;

        virtual void EngagementPlanInitializationAfterLaunch() = 0;
        virtual void UpdateEngagementPlanResult() = 0;
        virtual void SendEngagementPlanResult() = 0;

        // 발사 가능 구역 진입 여부 확인
        virtual void IsInValidLaunchGeometry() = 0;

        // 무장별 경로점 설정
        virtual void SetWaypoints() = 0;

        // AI 경로점 요청을 위한 명중지점 및 금지구역 정보 변환
        virtual void SetAIWaypointInferenceRequestMessage(AIEP_INTERNAL_INFER_REQ& RequestMsg) = 0;

        //
        virtual void ConvertAIWaypointsToGeodetic(const AIEP_INTERNAL_INFER_RESULT_WP& AIWPInferReq, AIEP_AI_INFER_RESULT_WP& msg) = 0;

        // 할당 정보 변경 여부 확인 및 반영
        virtual bool IsValidAssignmentInfo(const ST_WA_SESSION& weaponAssignInfo) = 0;
        virtual bool IsAssignmentInfoChanged(const ST_WA_SESSION& weaponAssignInfo) = 0;
        virtual void ApplyWeaponAssignmentInformation(const ST_WA_SESSION weaponAssignInfo) = 0;

        // 할당 정보
        ST_WA_SESSION m_weaponAssignmentInfo;

        // 멤버 변수
        std::shared_ptr<AIEP::DdsComm> m_ddsComm;

        std::atomic<bool> m_initialized{ false };
        std::atomic<bool> m_isLaunched{ false };
        std::atomic<bool> m_engagementPlanReady{ false };
        std::thread m_engagementPlanThread;
        int m_tubeNumber;
        uint32_t m_weaponKind;
        std::atomic<bool> m_shutdown{ false };
        WeaponSpecification m_weaponSpec;

        // 발사 시간
        std::chrono::steady_clock::time_point m_launchTime;

        // 환경 정보
        NAVINF_SHIP_NAVIGATION_INFO m_ownShipInfo;
        TRKMGR_SYSTEMTARGET_INFO m_targetInfo;
        CMSHCI_AIEP_PA_INFO m_paInfo;
        CMSHCI_AIEP_WPN_GEO_WAYPOINTS m_waypointCmd;

        std::mutex m_dataMutex;
    };
} // namespace AIEP
