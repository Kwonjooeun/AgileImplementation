#include "MineEngagementManager.h"
#include "../utils/AIEP_DataConverter.h"
#include "../utils/CCalcMethod.h"

#include <cstring>

namespace AIEP {

    // =============================================================================
    // MineEngagementManager 구현
    // =============================================================================

    MineEngagementManager::MineEngagementManager(ST_WA_SESSION weaponAssignInfo,
        std::shared_ptr<AIEP::DdsComm> ddsComm) :
        EngagementManagerBase(weaponAssignInfo, ddsComm),
        m_MineModel(std::make_unique<M_MINE_Model>())
    {
        memset(&m_dropPlan, 0, sizeof(m_dropPlan));

        m_dropPlanListNumber = weaponAssignInfo.usAllocDroppingPlanListNum()-1;
        m_dropPlanNumber = weaponAssignInfo.usAllocLayNum()-1;

        if (!LoadMineDropPlan(m_dropPlanListNumber, m_dropPlanNumber)) // m_dropPlan 
        {
            throw std::runtime_error("Fail to initialize drop plan and dynamics model of M_MINE.");
        }

        m_MineEngagementPlanResult_ENU.cachedPlanState = static_cast<int>(EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_ASSIGN);
        DroppingPlanManager->updatePlanState(MINE_PLAN_FILE, m_dropPlanListNumber, m_dropPlanNumber, EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_ASSIGN);

        SetupDynamicsModel();
        m_Max_sec_x10 = (int)(m_weaponSpec.maxRange_km * 1000.0 / m_weaponSpec.maxSpeed_mps * 10.0);
    }

    void MineEngagementManager::EngagementPlanInitializationAfterLaunch()
    {
        double Latitude, Longitude;
        float Altitude;
        SPOINT_WEAPON_ENU OwnshipPos_ENU;
        GEO_POINT_2D center{ 0, };

        if (m_MineEngagementPlanResult_ENU.cachedPlanState != static_cast<int>(EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_LAUNCH))
        {
            m_MineEngagementPlanResult_ENU.cachedPlanState = static_cast<int>(EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_LAUNCH);
            DroppingPlanManager->updatePlanState(MINE_PLAN_FILE, m_dropPlanListNumber, m_dropPlanNumber, EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_LAUNCH);
        }

        {
            std::lock_guard<std::mutex> lock(m_dataMutex);

            center.latitude = m_ownShipInfo.stShipMovementInfo().dShipLatitude();
            center.longitude = m_ownShipInfo.stShipMovementInfo().dShipLongitude();

            Latitude = m_ownShipInfo.stShipMovementInfo().dShipLatitude();
            Longitude = m_ownShipInfo.stShipMovementInfo().dShipLongitude();
            Altitude = -m_ownShipInfo.stUnderwaterEnvironmentInfo().fDivingDepth();
        }

        DataConverter::convertLatLonAltToLocal(center, Latitude, Longitude, Altitude, OwnshipPos_ENU);

        m_MineEngagementPlanResult_ENU.mslDRPos.E = OwnshipPos_ENU.E;
        m_MineEngagementPlanResult_ENU.mslDRPos.N = OwnshipPos_ENU.N;
        m_MineEngagementPlanResult_ENU.mslDRPos.U = OwnshipPos_ENU.U;

        m_MineEngagementPlanResult_ENU.idxOfNextWP = 1;
        m_MineEngagementPlanResult_ENU.timeSinceLaunch_sec = 0.;
    }

    void MineEngagementManager::UpdateEngagementPlanResult() {
        if (m_shutdown.load()) return;

        try {
            if (m_isLaunched) // 발사 후
            {
                EstimateCurrentStatus();
            }
            else // 발사 전
            {
                SetupDynamicsModel();
                PlanTrajectory();

                if (m_dropPlanLoaded && m_dropPlanValid && isInLaunchableArea.load()) {
                    m_engagementPlanReady.store(true);
                }
                else
                {
                    m_engagementPlanReady.store(false);
                }
            }
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(ENGAGEMENT) << "Mine engagement plan calculation error: " << e.what() << std::endl;
        }
    }

    void MineEngagementManager::SendEngagementPlanResult() {
        if (!m_ddsComm || m_shutdown.load()) return;

        try {
            AIEP_M_MINE_EP_RESULT result;
            std::array<ST_WEAPON_WAYPOINT, 8> arrGeoWaypoint;
            std::array<float, 8> arrWaypointArrivalTime;
            std::array<ST_3D_GEODETIC_POSITION, 128> arrTrajectory;
            std::vector<ST_3D_GEODETIC_POSITION> vecGeoTrajectory;

            GEO_POINT_2D center{ 0, };
            {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                center.latitude = m_ownShipInfo.stShipMovementInfo().dShipLatitude();
                center.longitude = m_ownShipInfo.stShipMovementInfo().dShipLongitude();

                result.enTubeNum() = static_cast<uint32_t>(m_tubeNumber);
                result.unCntWaypoint() = (unsigned short)m_Geowaypoints.size();

                std::copy(m_Geowaypoints.begin(), m_Geowaypoints.end(), arrGeoWaypoint.begin());
                result.stWaypoints(arrGeoWaypoint);

                std::copy(m_MineEngagementPlanResult_ENU.waypointsArrivalTimes.begin(), m_MineEngagementPlanResult_ENU.waypointsArrivalTimes.end(), arrWaypointArrivalTime.begin());
                result.waypointArrivalTime(arrWaypointArrivalTime);

                result.bValidMslPos() = (bool)m_MineEngagementPlanResult_ENU.bValidMslDRPos;
                DataConverter::convertLocalENToLatLon(center, m_MineEngagementPlanResult_ENU.mslDRPos.E, m_MineEngagementPlanResult_ENU.mslDRPos.N, result.MslPos().dLatitude(), result.MslPos().dLongitude());
                result.MslPos().fDepth() = (float)-m_MineEngagementPlanResult_ENU.mslDRPos.U;

                result.numberOfNextWP() = m_MineEngagementPlanResult_ENU.idxOfNextWP - 1;
                result.timeToNextWP() = m_MineEngagementPlanResult_ENU.timeToNextWP;
                result.unCntTrajectory() = (unsigned short)m_MineEngagementPlanResult_ENU.number_of_trajectory;

                DataConverter::convertLocalArrToGeo(center, m_MineEngagementPlanResult_ENU.trajectory, vecGeoTrajectory);
                std::copy(vecGeoTrajectory.begin(), vecGeoTrajectory.end(), arrTrajectory.begin());
                result.stTrajectories(arrTrajectory);

                result.fEstimatedDrivingTime() = m_MineEngagementPlanResult_ENU.time_to_destination;
                result.fRemainingTime() = m_MineEngagementPlanResult_ENU.RemainingTime;

                result.stDropPos().dLatitude() = m_dropPlan.stDropPos().dLatitude();
                result.stDropPos().dLongitude() = m_dropPlan.stDropPos().dLongitude();
                result.stDropPos().fDepth() = m_dropPlan.stDropPos().fDepth();

                result.stLaunchPos().dLatitude() = m_dropPlan.stLaunchPos().dLatitude();
                result.stLaunchPos().dLongitude() = m_dropPlan.stLaunchPos().dLongitude();
                result.stLaunchPos().fDepth() = m_dropPlan.stLaunchPos().fDepth();

                result.sBatteryCapacity() = (short)m_MineEngagementPlanResult_ENU.BatteryCapacity_percentage;
                result.fBatteryTime() = (float)m_MineEngagementPlanResult_ENU.BatteryTime_sec;
            }
            m_ddsComm->Send(result);
        }
        catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(ENGAGEMENT) << "Failed to send mine engagement result: " << e.what() << std::endl;
        }
    }

    // < 발사 전 > 교전계획 계산
    void MineEngagementManager::PlanTrajectory()
    {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_MineEngagementPlanResult_ENU.resetTrajectoryInfo();

        m_MineEngagementPlanResult_ENU.BatteryCapacity_percentage = 100; // 실제 탄 연동 시에는 실제 축전량으로 반영해야 함
        m_MineEngagementPlanResult_ENU.BatteryTime_sec = m_energyConsumtionperSec;

        int nextWaypointIdx{ m_MineEngagementPlanResult_ENU.idxOfNextWP };
        bool bDestinationReached = false;
        std::vector<SPOINT_ENU> fullTrajectory;
        fullTrajectory.reserve(m_Max_sec_x10);

        SPOINT_ENU LaunchPoint{ m_MineEngagementPlanResult_ENU.LaunchPoint.E, m_MineEngagementPlanResult_ENU.LaunchPoint.N , m_MineEngagementPlanResult_ENU.LaunchPoint.U };
        m_MineEngagementPlanResult_ENU.mslDRPos = LaunchPoint;

        for (int i = 0; i < m_Max_sec_x10; i++)
        {
            int backup_next_wp_to_go = nextWaypointIdx;

            // m_Max_sec_x10 값이 0.1 sec 단위 시간 기준임
            bDestinationReached = m_MineModel->runWaypoints(0.1, nextWaypointIdx, m_MineEngagementPlanResult_ENU.mslDRPos); // dead reckoning 미반영한 단위 시간 기동

            if (backup_next_wp_to_go != nextWaypointIdx) // WP reached and next wp updated
            {
                if (backup_next_wp_to_go > 0)
                    m_MineEngagementPlanResult_ENU.waypointsArrivalTimes.push_back((float)(i / 10.0));
            }

            fullTrajectory.push_back(m_MineEngagementPlanResult_ENU.mslDRPos);

            if (bDestinationReached) // 도착 지점 도달 시,
            {
                m_MineEngagementPlanResult_ENU.time_to_destination = i / 10.0;
                break;
            }
        }

        m_MineEngagementPlanResult_ENU.mslDRPos = LaunchPoint;

        if (m_MineEngagementPlanResult_ENU.time_to_destination == 0.) // 최대 사거리를 벗어나는 교전계획이란 의미
        {
            m_dropPlanValid = false;

            if (m_MineEngagementPlanResult_ENU.cachedPlanState != static_cast<int>(EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_ERROR))
            {
                m_MineEngagementPlanResult_ENU.cachedPlanState = static_cast<int>(EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_ERROR);
                DroppingPlanManager->updatePlanState(MINE_PLAN_FILE, m_dropPlanListNumber, m_dropPlanNumber, EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_ERROR);
            }
        }
        else
        {
            m_dropPlanValid = true;

            if (m_MineEngagementPlanResult_ENU.cachedPlanState != static_cast<int>(EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_PLAN))
            {
                m_MineEngagementPlanResult_ENU.cachedPlanState = static_cast<int>(EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_PLAN);
                DroppingPlanManager->updatePlanState(MINE_PLAN_FILE, m_dropPlanListNumber, m_dropPlanNumber, EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_PLAN);
            }
        }

        double step = static_cast<double>(fullTrajectory.size() - 1) / (m_weaponSpec.trajectoryArrayLength - 1);

        // 점 추출
        for (size_t i = 0; i < m_weaponSpec.trajectoryArrayLength; ++i)
        {
            size_t index = static_cast<size_t>(std::round(i * step));
            if (index >= fullTrajectory.size())
                break;
            m_MineEngagementPlanResult_ENU.trajectory.push_back(fullTrajectory[index]);
            m_MineEngagementPlanResult_ENU.flightTimeOfTrajectory.push_back((float)(index * 0.1)); // full trajectory is recorded every 0.1 sec
            ++m_MineEngagementPlanResult_ENU.number_of_trajectory;
        }
    }

    // < 발사 후 > 탄 위치 예측
    void MineEngagementManager::EstimateCurrentStatus()
    {
        float ElapsedTimeafterLaunch{ m_MineEngagementPlanResult_ENU.timeSinceLaunch_sec };
        int IdxofNextWP{ m_MineEngagementPlanResult_ENU.idxOfNextWP };

        ElapsedTimeafterLaunch = ElapsedTimeafterLaunch + m_weaponSpec.engagementPlanUpdateInterval;

        if (m_MineModel->runWaypoints(m_weaponSpec.engagementPlanUpdateInterval, IdxofNextWP, m_MineEngagementPlanResult_ENU.mslDRPos)) // 부설완료
        {
            m_MineEngagementPlanResult_ENU.RemainingTime = 0.;

            if (m_MineEngagementPlanResult_ENU.cachedPlanState != static_cast<int>(EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_FINISH))
            {
                m_MineEngagementPlanResult_ENU.cachedPlanState = static_cast<int>(EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_FINISH);
                DroppingPlanManager->updatePlanState(MINE_PLAN_FILE, m_dropPlanListNumber, m_dropPlanNumber, EN_M_MINE_PLAN_STATE::M_MINE_PLAN_STATE_FINISH);
            }
        }
        else
        {
            // 축전 잔여량 계산
            float InitialBatteryCapacity_Wh = (float)(m_InitialBatteryCapacity_percentage * M_MINE_MAX_BATTERY_CAPACITY_WH) / 100.; // [Wh]

            float totalEnergyConsumed_Wh{ m_MineEngagementPlanResult_ENU.TotalEnergyConsumed_Wh };
            float currentBatteryCapacity_Wh{ 0. };

            totalEnergyConsumed_Wh = totalEnergyConsumed_Wh + m_energyConsumtionperSec * m_weaponSpec.engagementPlanUpdateInterval;
            currentBatteryCapacity_Wh = InitialBatteryCapacity_Wh - totalEnergyConsumed_Wh;

            if (InitialBatteryCapacity_Wh != 0)
            {
                m_MineEngagementPlanResult_ENU.BatteryCapacity_percentage = (currentBatteryCapacity_Wh / InitialBatteryCapacity_Wh) * 100;
                m_MineEngagementPlanResult_ENU.BatteryTime_sec = InitialBatteryCapacity_Wh / m_energyConsumtionperSec - ElapsedTimeafterLaunch;
            }
            else
            {
                m_MineEngagementPlanResult_ENU.BatteryCapacity_percentage = 0;
                m_MineEngagementPlanResult_ENU.BatteryTime_sec = 0.;
            }

            // 부설 지점까지 남은 시간
            m_MineEngagementPlanResult_ENU.RemainingTime = m_MineEngagementPlanResult_ENU.time_to_destination - ElapsedTimeafterLaunch;

            if (IdxofNextWP <= m_MineEngagementPlanResult_ENU.number_of_valid_waypoint)
            {
                // 다음 경로점까지 남은 시간
                m_MineEngagementPlanResult_ENU.timeToNextWP = m_MineEngagementPlanResult_ENU.waypointsArrivalTimes.at(IdxofNextWP - 1) - ElapsedTimeafterLaunch;
            }
            else
            {
                m_MineEngagementPlanResult_ENU.timeToNextWP = m_MineEngagementPlanResult_ENU.RemainingTime;
            }
        }
        m_MineEngagementPlanResult_ENU.timeSinceLaunch_sec = ElapsedTimeafterLaunch;
        m_MineEngagementPlanResult_ENU.idxOfNextWP = IdxofNextWP;
        m_MineEngagementPlanResult_ENU.bValidMslDRPos = 1;
    }

    void MineEngagementManager::IsInValidLaunchGeometry()
    {
        GEO_POINT_2D center{ 0, };
        std::vector<SPOINT_WEAPON_ENU> localRoute;
        SPOINT_WEAPON_ENU OwnshipPos;
        SPOINT_WEAPON_ENU DropPos;
        SPOINT_WEAPON_ENU Waypoint;

        {
            std::lock_guard<std::mutex> lockdata(m_dataMutex);
            center.latitude = m_ownShipInfo.stShipMovementInfo().dShipLatitude();
            center.longitude = m_ownShipInfo.stShipMovementInfo().dShipLongitude();

            DataConverter::convertLatLonAltToLocal(
                center, 
                m_ownShipInfo.stShipMovementInfo().dShipLatitude(), 
                m_ownShipInfo.stShipMovementInfo().dShipLongitude(), 
                m_ownShipInfo.stUnderwaterEnvironmentInfo().fDivingDepth(), 
                OwnshipPos);

            localRoute.push_back(OwnshipPos); // 순서 중요

            for (int i = 0; i < m_Geowaypoints.size(); i++)
            {
                if (m_Geowaypoints.at(i).bValid())
                {
                    memset(&Waypoint, 0, sizeof(Waypoint));

                    DataConverter::convertLatLonAltToLocal(
                        center, 
                        m_Geowaypoints.at(i).dLatitude(), 
                        m_Geowaypoints.at(i).dLongitude(), 
                        m_Geowaypoints.at(i).fDepth(), 
                        Waypoint);

                    localRoute.push_back(Waypoint); // 순서 중요
                }
            }
            std::lock_guard<std::mutex> lockplan(m_planMutex);
            DataConverter::convertLatLonAltToLocal(center
                , m_dropPlan.stDropPos().dLatitude()
                , m_dropPlan.stDropPos().dLongitude()
                , m_dropPlan.stDropPos().fDepth()
                , DropPos);

            localRoute.push_back(DropPos); // 순서 중요
        }

        double totalDistancefromWP1toDestination{ 0. };
        for (int i = 1;i< localRoute.size() - 1; i++)
        {
            double distancefromAtoB{ 0. };
            distancefromAtoB = CCalcMethod::GetDistance(
                localRoute[i].E, 
                localRoute[i].N,
                localRoute[i + 1].E,
                localRoute[i + 1].N);
            totalDistancefromWP1toDestination += distancefromAtoB;
        }

        double distancefromWP1toDestination{m_weaponSpec.maxRange_km * 1000. - totalDistancefromWP1toDestination};


        double distancefromOwnshiptoWP1{ 0. };
        double anglefromWP1toOwnship{ 0. };

        CCalcMethod::GetRangeBearing(
            -localRoute[1].E,
            -localRoute[1].N,
            distancefromOwnshiptoWP1,
            anglefromWP1toOwnship);

        if (distancefromWP1toDestination >= distancefromOwnshiptoWP1)
        {
            double anglefromWP1toNext{ 0. };
            double distancefromWP1toNext{ 0. };

            CCalcMethod::GetRangeBearing(
                localRoute[2].E - localRoute[1].E,
                localRoute[2].N - localRoute[1].N,
                distancefromWP1toNext,
                anglefromWP1toNext);

            double AngleA = anglefromWP1toNext + 90.;
            double AngleB = anglefromWP1toNext - 90.;

            bool bTrans = false;
            if (AngleB < 0.)
            {
                AngleB += 360.;
                bTrans = true;
            }
            if (AngleA > 360.)
            {
                AngleA -= 360.;
                bTrans = true;
            }

            bool bAng = false;
            if (bTrans == false)
            {
                if (AngleA > AngleB)
                {
                    if ((anglefromWP1toOwnship < AngleB) || (anglefromWP1toOwnship > AngleA))
                    {
                        bAng = true;
                    }
                }
                else
                                    {
                    if ((anglefromWP1toOwnship < AngleA) || (anglefromWP1toOwnship > AngleB))
                    {
                        bAng = true;
                    }
                }
            }
            else
            {
                if (AngleA > AngleB)
                {
                    if ((anglefromWP1toOwnship > AngleB) && (anglefromWP1toOwnship < AngleA))
                    {
                        bAng = true;
                    }
                }
                else
                {
                    if ((anglefromWP1toOwnship > AngleA) && (anglefromWP1toOwnship < AngleB))
                    {
                        bAng = true;
                    }
                }
            }
            if (bAng)
            {
                isInLaunchableArea.store(true);
            }
            else
            {
                isInLaunchableArea.store(false);
            }
        }
        else 
        {
            isInLaunchableArea.store(false);
        }
        SetLaunchPoint();
    }

    void MineEngagementManager::SetWaypoints()
    {
        if (m_waypointCmd.stGeoWaypoints().unCntWaypoints() < 0 ||
            m_waypointCmd.stGeoWaypoints().unCntWaypoints() > 8)
        {
            return;
        }
        else
        {
            m_Geowaypoints.clear();
            for (int i = 0; i < m_waypointCmd.stGeoWaypoints().unCntWaypoints(); i++)
            {
                m_Geowaypoints.push_back(m_waypointCmd.stGeoWaypoints().stGeoPos()[i]);
            }
        }

    }

    void MineEngagementManager::SetLaunchPoint()
    {
        if (isInLaunchableArea.load())
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            LaunchPos_Geo.dblLatitude() = m_ownShipInfo.stShipMovementInfo().dShipLatitude();
            LaunchPos_Geo.dblLongitude() = m_ownShipInfo.stShipMovementInfo().dShipLongitude();
            LaunchPos_Geo.fAltitude() = -m_ownShipInfo.stUnderwaterEnvironmentInfo().fDivingDepth();
        }
        else
        {
            std::lock_guard<std::mutex> lock(m_planMutex);
            LaunchPos_Geo.dblLatitude() = m_dropPlan.stLaunchPos().dLatitude();
            LaunchPos_Geo.dblLongitude() = m_dropPlan.stLaunchPos().dLongitude();
            LaunchPos_Geo.fAltitude() = -m_dropPlan.stLaunchPos().fDepth();
        }
    }

    void MineEngagementManager::SetAIWaypointInferenceRequestMessage(AIEP_INTERNAL_INFER_REQ& RequestMsg)
    {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        GEO_POINT_2D center;

        center.latitude = LaunchPos_Geo.dblLatitude();
        center.longitude = LaunchPos_Geo.dblLongitude();

        RequestMsg.eTubeNum() = static_cast<uint32_t>(m_tubeNumber);
        RequestMsg.ReqType() = static_cast<uint16_t>(m_weaponKind);

        SPOINT_WEAPON_ENU TargetPos_ENU;
        DataConverter::convertLatLonAltToLocal(center, TargetPos_Geo.dblLatitude(), TargetPos_Geo.dblLongitude(), TargetPos_Geo.fAltitude(), TargetPos_ENU);
        RequestMsg.TargetPosition().fPositionD() = -TargetPos_ENU.U;
        RequestMsg.TargetPosition().fPositionE() = TargetPos_ENU.E;
        RequestMsg.TargetPosition().fPositionN() = TargetPos_ENU.N;

        RequestMsg.TargetCourse() = 0.;
        RequestMsg.TargetSpeed() = 0.;
        RequestMsg.PaCount() = m_paInfo.nCountPA();

        SPOINT_WEAPON_ENU PAPos_ENU;
        ST_PA_POINT_ENU PAInfo_ENU;
        std::array<ST_PA_POINT_ENU, 16> PAInfoArr_ENU;
        std::vector<ST_PA_POINT_ENU> PAInfoVec_ENU;
        float dummy{ 0. };
        for (int i = 0; i < m_paInfo.nCountPA(); i++)
        {
            DataConverter::convertLatLonAltToLocal(center, m_paInfo.stPaPoint()[i].dLatitude(), m_paInfo.stPaPoint()[i].dLongitude(), dummy, PAPos_ENU);
            PAInfo_ENU.E() = PAPos_ENU.E;
            PAInfo_ENU.N() = PAPos_ENU.N;
            PAInfo_ENU.U() = PAPos_ENU.U;
            PAInfo_ENU.speed() = m_paInfo.stPaPoint()[i].dSpeed();
            PAInfo_ENU.course() = m_paInfo.stPaPoint()[i].dCourse();
            PAInfo_ENU.radius() = m_paInfo.stPaPoint()[i].dRadius();

            PAInfoVec_ENU.push_back(PAInfo_ENU);
        }
        std::copy(PAInfoVec_ENU.begin(), PAInfoVec_ENU.end(), PAInfoArr_ENU.begin());
        RequestMsg.PAInfo(PAInfoArr_ENU);
        RequestMsg.ImpactAngle() = 0.;
    }

    void MineEngagementManager::ConvertAIWaypointsToGeodetic(const AIEP_INTERNAL_INFER_RESULT_WP& AIWPInferReq, AIEP_AI_INFER_RESULT_WP& msg)
    {
        GEO_POINT_2D center;
        float dropDepth, launchDepth;
        dropDepth = -TargetPos_Geo.fAltitude();
        launchDepth = -LaunchPos_Geo.fAltitude();

        float depthStepSize{ (float)((dropDepth - launchDepth) / (AIWPInferReq.CountWaypoints())) };

        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            center.latitude = LaunchPos_Geo.dblLatitude();
            center.longitude = LaunchPos_Geo.dblLongitude();
        }

        msg.eTubeNum() = m_tubeNumber;
        msg.eWpnKind() = static_cast<int32_t>(m_weaponKind);
        msg.stGeoWaypoints().unCntWaypoints() = AIWPInferReq.CountWaypoints() - 1; // AI 경로점의 마지막 point는 target 위치이므로 제거

        for (int i = 0; i < AIWPInferReq.CountWaypoints() -1; i++)
        {
            msg.stGeoWaypoints().stGeoPos()[i].bValid() = 1;
            msg.stGeoWaypoints().stGeoPos()[i].fSpeed() = 0.;

            DataConverter::convertLocalENToLatLon(
                center, 
                AIWPInferReq.Waypoints()[i].fPositionE(), 
                AIWPInferReq.Waypoints()[i].fPositionN(),
                msg.stGeoWaypoints().stGeoPos()[i].dLatitude(), 
                msg.stGeoWaypoints().stGeoPos()[i].dLongitude());
            
            msg.stGeoWaypoints().stGeoPos()[i].fDepth() = launchDepth + depthStepSize * (i + 1);
        }
    }

    bool MineEngagementManager::IsValidAssignmentInfo(const ST_WA_SESSION& weaponAssignInfo)
    {
        if (weaponAssignInfo.usAllocDroppingPlanListNum() && weaponAssignInfo.usAllocLayNum())
        {
            uint32_t newDropPlanListNum{ weaponAssignInfo.usAllocDroppingPlanListNum() };
            uint32_t newDropPlanNum{ weaponAssignInfo.usAllocLayNum() };

            // 부설계획 목록과 부설계획은 각각 최대 15개까지 수립가능
            if (newDropPlanListNum < 1 || newDropPlanListNum > 15)
            {
                return false;
            }

            if (newDropPlanNum < 1 || newDropPlanNum > 15)
            {
                return false;
            }

            return true;
        }
        else
        {
            return false;
        }
    }

    bool MineEngagementManager::IsAssignmentInfoChanged(const ST_WA_SESSION& weaponAssignInfo)
    {
        uint32_t newDropPlanListNum{ uint32_t(weaponAssignInfo.usAllocDroppingPlanListNum() - 1)};
        uint32_t newDropPlanNum{ uint32_t(weaponAssignInfo.usAllocLayNum() - 1)};
        return !(m_dropPlanListNumber == newDropPlanListNum && m_dropPlanNumber == newDropPlanNum);
    }

    void MineEngagementManager::ApplyWeaponAssignmentInformation(const ST_WA_SESSION weaponAssignInfo)
    {
        {
                        std::lock_guard<std::mutex> lock(m_planMutex);
            m_dropPlanListNumber = weaponAssignInfo.usAllocDroppingPlanListNum();
            m_dropPlanNumber = weaponAssignInfo.usAllocLayNum();
        }
        if (!LoadMineDropPlan(m_dropPlanListNumber, m_dropPlanNumber)) // m_dropPlan 
        {
            DEBUG_ERROR_STREAM(ENGAGEMENT) << "Fail to update the drop plan " << std::endl;
        }
    }

    void MineEngagementManager::SetupDynamicsModel()
    {
        if (!m_dropPlanLoaded) return;

        std::vector<SPOINT_WEAPON_ENU> FullRoutePoints;
        FullRoutePoints.clear();

        int MaximumNbrOfWPs{ m_weaponSpec.maxWaypoints };
        
        double Latitude, Longitude;
        float Altitude;

        SPOINT_WEAPON_ENU LaunchPoint, DropPoint, Waypoint;
        std::vector<SPOINT_WEAPON_ENU> Waypoints;
        Waypoints.reserve(MaximumNbrOfWPs);

        GEO_POINT_2D center{ 0, };        
        {
            std::lock_guard<std::mutex> datalock(m_dataMutex);
            std::lock_guard<std::mutex> planlock(m_planMutex);

            center.latitude = m_ownShipInfo.stShipMovementInfo().dShipLatitude();
            center.longitude = m_ownShipInfo.stShipMovementInfo().dShipLongitude();        

            // Launch Point 변환
            Latitude = LaunchPos_Geo.dblLatitude();
            Longitude = LaunchPos_Geo.dblLongitude();
            Altitude = LaunchPos_Geo.fAltitude();        
            DataConverter::convertLatLonAltToLocal(center, Latitude, Longitude, Altitude, LaunchPoint);
            FullRoutePoints.push_back(LaunchPoint); // 순서 중요 ( Launch point -> Waypoints -> Drop point)        

            // Waypoints 저장
            for (int i = 0; i < m_Geowaypoints.size(); i++)
            {
                if (m_Geowaypoints.at(i).bValid())
                {
                    memset(&Waypoint, 0, sizeof(Waypoint));

                    Latitude = m_Geowaypoints.at(i).dLatitude();
                    Longitude = m_Geowaypoints.at(i).dLongitude();
                    Altitude = -m_Geowaypoints.at(i).fDepth();
                    DataConverter::convertLatLonAltToLocal(center, Latitude, Longitude, Altitude, Waypoint);
                    Waypoint.Validation = static_cast<bool>(m_Geowaypoints.at(i).bValid());

                    Waypoints.push_back(Waypoint);
                    FullRoutePoints.push_back(Waypoint); // 순서 중요
                }
            }

            // Drop Point 저장
            Latitude = m_dropPlan.stDropPos().dLatitude();
            Longitude = m_dropPlan.stDropPos().dLongitude();
            Altitude = -m_dropPlan.stDropPos().fDepth();
            DataConverter::convertLatLonAltToLocal(center, Latitude, Longitude, Altitude, DropPoint);
            FullRoutePoints.push_back(DropPoint); // 순서 중요

            m_MineEngagementPlanResult_ENU.DropPoint = DropPoint;
            m_MineEngagementPlanResult_ENU.LaunchPoint = LaunchPoint;
            m_MineEngagementPlanResult_ENU.waypoints = Waypoints;
            m_MineEngagementPlanResult_ENU.number_of_valid_waypoint = Waypoints.size();
        }

        m_MineModel->SetFullRoutePoints(FullRoutePoints);
    }

    void MineEngagementManager::SetTarget(const double& Latitude, const double& Longitude, const float& Altitude)
    {
                std::lock_guard<std::mutex> lock(m_dataMutex);
        TargetPos_Geo.dblLatitude() = Latitude;
        TargetPos_Geo.dblLongitude() = Longitude;
        TargetPos_Geo.fAltitude() = Altitude;
    }

    bool MineEngagementManager::LoadMineDropPlan(const uint32_t listNum, const uint32_t planNum) {
        ST_M_MINE_PLAN_INFO LoadedPlan;
        std::lock_guard<std::mutex> lock(m_planMutex);

        // m_dropPlan에 부설계획 로드
        try
        {
            if (listNum >= 0 && planNum >= 0)
                DroppingPlanManager->readDroppingPlanfromFile(MINE_PLAN_FILE, listNum, planNum, LoadedPlan);
            else
                return false;

            if (LoadedPlan.usDroppingPlanNumber() != 0) // 존재하는 부설계획인가?
            {
                m_dropPlanLoaded = true;
                m_dropPlan = LoadedPlan;

                m_Geowaypoints.clear();
                for (int i = 0; i < LoadedPlan.usWaypointCnt(); i++)
                {
                    m_Geowaypoints.push_back(LoadedPlan.stWaypoint()[i]);
                }

                SetTarget(LoadedPlan.stDropPos().dLatitude(), LoadedPlan.stDropPos().dLongitude(), -LoadedPlan.stDropPos().fDepth());

                DEBUG_STREAM(ENGAGEMENT) << "Mine drop plan set for Tube " << m_tubeNumber
                    << " (List: " << listNum << ", Plan: " << planNum << ")" << std::endl;

                return true;
            }
            else
            {
                DEBUG_ERROR_STREAM(ENGAGEMENT) << "Mine dropping plan" << planNum
                    << " in plan list " << listNum << " does not exist " << std::endl;

                return false;
            }
        }
                   catch (const std::exception& e) {
            DEBUG_ERROR_STREAM(ENGAGEMENT) << "Mine drop plan setting Fail: " << e.what() << std::endl;
            m_dropPlanLoaded = false;

            return false;
        }        
    }

    bool MineEngagementManager::UpdateDropPlanWaypoints(const std::vector<ST_WEAPON_WAYPOINT>& waypoints) {
        std::lock_guard<std::mutex> lock(m_planMutex);

        if (waypoints.size() > 8) {
            DEBUG_ERROR_STREAM(ENGAGEMENT) << "Too many waypoints for mine drop plan: " << waypoints.size() << std::endl;
            return false;
        }

        // 경로점 업데이트
        m_dropPlan.usWaypointCnt() = static_cast<unsigned short>(waypoints.size());
        for (size_t i = 0; i < waypoints.size(); ++i) {
            m_dropPlan.stWaypoint()[i] = waypoints[i];
        }

        DEBUG_STREAM(ENGAGEMENT) << "Mine drop plan waypoints updated for Tube " << m_tubeNumber
            << ", waypoint count: " << waypoints.size() << std::endl;
        return true;
    }

} // namespace AIEP
