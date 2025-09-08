#pragma once

#include "../../../dds_message/AIEP_AIEP_.hpp"
#include "../../../Common/Utils/ConfigManager.h"
#include "../EngagementManagerBase.h"

#include "M_MINE_DroppingPlanManager/M_Mine_DroppingPlanManager.h"
#include "M_MINE_Model/M_MINE_Model.h"
#include <memory>
#include <vector>
#include <chrono>

namespace AIEP {
    // =============================================================================
    // 자항기뢰 전용 교전계획 관리자
    // =============================================================================
    class MineEngagementManager : public EngagementManagerBase {
    public:
        MineEngagementManager(ST_WA_SESSION weaponAssignInfo, std::shared_ptr<AIEP::DdsComm> ddsComm);
        ~MineEngagementManager() = default;

    protected:
        // EngagementManagerBase 구현
        void EngagementPlanInitializationAfterLaunch() override;
        void UpdateEngagementPlanResult() override;
        void SendEngagementPlanResult() override;

        void PlanTrajectory();
        void EstimateCurrentStatus();

        void IsInValidLaunchGeometry() override;

        void SetWaypoints() override;
        void SetLaunchPoint();
        void SetAIWaypointInferenceRequestMessage(AIEP_INTERNAL_INFER_REQ& RequestMsg) override;
        void ConvertAIWaypointsToGeodetic(const AIEP_INTERNAL_INFER_RESULT_WP& AIWPInferReq, AIEP_AI_INFER_RESULT_WP& msg) override;

        bool IsValidAssignmentInfo(const ST_WA_SESSION& weaponAssignInfo) override;
        bool IsAssignmentInfoChanged(const ST_WA_SESSION& weaponAssignInfo) override;
        void ApplyWeaponAssignmentInformation(const ST_WA_SESSION weaponAssignInfo) override;



    private:       
        // 멤버 변수
        int m_Max_sec_x10{ 0 };
        std::atomic<bool> isInLaunchableArea{ false }; // 발사 가능 구역 내 자함 존재 여부
        SAL_MINE_EP_RESULT m_MineEngagementPlanResult_ENU; // 교전계획 산출 결과 coord: ENU
        std::vector<ST_WEAPON_WAYPOINT> m_Geowaypoints; //  경로점만 따로 관리 
        SGEODETIC_POSITION LaunchPos_Geo;
        SGEODETIC_POSITION TargetPos_Geo;

        // 부설계획 좌표계 변환(Geo->ENU) 및 자항기뢰 모델 초기화
        void SetupDynamicsModel();

        // 부설지점 설정
        void SetTarget(const double& Latitude, const double& Longitude, const float& Altitude);

        // ==========================================================================
        // 부설계획 관리
        // ==========================================================================
        M_MineDroppingPlanManager* DroppingPlanManager;
        std::unique_ptr< M_MINE_Model> m_MineModel;

        // 부설계획 정보
        ST_M_MINE_PLAN_INFO m_dropPlan; // 부설 계획 캐시
        bool m_dropPlanLoaded{ false };
        bool m_dropPlanValid{ false };
        uint32_t m_dropPlanListNumber{ 0 }; // 부설 계획 목록 번호
        uint32_t m_dropPlanNumber{ 0 }; // 부설 계획 번호
        mutable std::mutex m_planMutex;

        bool LoadMineDropPlan(const uint32_t listNum, const uint32_t planNum); // json 파일에서 부설 계획 로드 후 m_dropPlan에 저장
        bool UpdateDropPlanWaypoints(const std::vector<ST_WEAPON_WAYPOINT>& waypoints); // 경로점 수정 명령으로 인한 경로점 수정

        // config.ini 내에서 무장제원으로 분리하면 더 좋음
        const std::string MINE_PLAN_FILE = "Hello.json"; // 부설계획 파일 이름
        const float M_MINE_MAX_BATTERY_CAPACITY_WH = 16941.47; // [Wh]
        const float M_MINE_POWER_CONSUMPTION_COEFFICIENT_PER_HOUR = 3.9219; // [Wh/h/knot^3]

        const int m_InitialBatteryCapacity_percentage{ 100 };
        const float m_initialBatteryCapacity_Wh{ (float)(m_InitialBatteryCapacity_percentage * M_MINE_MAX_BATTERY_CAPACITY_WH / 100.) }; // [Wh] 
        const float m_energyConsumtionperSec{ float((M_MINE_POWER_CONSUMPTION_COEFFICIENT_PER_HOUR / 3600.f) * ((m_weaponSpec.maxSpeed_mps * MS2KN) * (m_weaponSpec.maxSpeed_mps * MS2KN) * (m_weaponSpec.maxSpeed_mps * MS2KN))) };

    };

} // namespace AIEP
