#include "M_MINE_Model.h"

namespace AIEP {
	M_MINE_Model::M_MINE_Model()
	{	
		memset(&m_DropPos, 0, sizeof(m_DropPos));
		memset(&m_LaunchPos, 0, sizeof(m_LaunchPos));
        m_weaponKind = static_cast<uint32_t>(EWF_WEAPON_TYPE::WF_WEAPON_TYPE_MMINE);
        auto& config = ConfigManager::GetInstance();
        m_weaponSpec = config.GetWeaponSpec(m_weaponKind);
	}

	M_MINE_Model::~M_MINE_Model()
	{

	}

	void M_MINE_Model::SetDropPosition(const SPOINT_WEAPON_ENU i_stDropPos)
	{
		
	}

	void M_MINE_Model::SetLaunchPosition(const SPOINT_WEAPON_ENU i_stLaunchPos)
	{
		
	}

	void M_MINE_Model::SetMineWayPointInfo(const std::vector<SPOINT_WEAPON_ENU>& i_stWP)
	{
		
	}

    void M_MINE_Model::SetFullRoutePoints(const std::vector<SPOINT_WEAPON_ENU>& i_stFullRoutes)
    {
        m_FullRoutePoints = i_stFullRoutes;
    }

    bool M_MINE_Model::runWaypoints(const float unitTIme, int& o_NextWPToGo, SPOINT_ENU& currentPos)
    {
        bool IsDestinationReached{ false };
        
        int IdxofNextWP{ o_NextWPToGo };

        if (m_FullRoutePoints.size() == 0)
        {
            throw std::runtime_error("LP, WPs, DP are not set");
        }
        else
        {
            runKinematicTowardWaypoint(unitTIme, IdxofNextWP, IsDestinationReached, currentPos);
            if (((m_FullRoutePoints.size() - 1) == IdxofNextWP) && IsDestinationReached) // 부설 지점 도착
            {
                return IsDestinationReached;
            }
            else if (IsDestinationReached)
            {
                ++IdxofNextWP;
                o_NextWPToGo = IdxofNextWP;
                IsDestinationReached = false;
            }
            else
                          {
            }
            return IsDestinationReached;
        }
    }

    void M_MINE_Model::runKinematicTowardWaypoint(const float unitTIme, int& o_NextWPToGo, bool& o_bWaypointReached, SPOINT_ENU& currentPos)
    {
        int IdxofNextWP{ o_NextWPToGo };

        // 현재 위치
        double msl_pos[3] = { 0., }; // {E, N, U}
        msl_pos[0] = currentPos.E;
        msl_pos[1] = currentPos.N;
        msl_pos[2] = currentPos.U;

        // 목표 waypoint 위치
        double next_waypoint_pos[3] = {
            m_FullRoutePoints[IdxofNextWP].E,  // E
            m_FullRoutePoints[IdxofNextWP].N,  // N
            m_FullRoutePoints[IdxofNextWP].U   // U 
        };

        // 방향 벡터 계산
        double dE = next_waypoint_pos[0] - msl_pos[0];
        double dN = next_waypoint_pos[1] - msl_pos[1];
        double dU = next_waypoint_pos[2] - msl_pos[2];

        // 거리 계산
        double distance_to_wp = sqrt(dE * dE + dN * dN + dU * dU);

        // 단위 벡터 계산
        double unit_dE = dE / distance_to_wp;
        double unit_dN = dN / distance_to_wp;
        double unit_dU = dU / distance_to_wp;

        // 이동 거리 계산
        double distance = m_weaponSpec.maxSpeed_mps * unitTIme;

        // 각 방향으로 이동
        double updated_E = msl_pos[0] + distance * unit_dE;
        double updated_N = msl_pos[1] + distance * unit_dN;
        double updated_U = msl_pos[2] + distance * unit_dU;

        // 위치 갱신
        currentPos.E = updated_E;
        currentPos.N = updated_N;
        currentPos.U = updated_U;

        // 도달 판정 (반경 10m 이내면 도달로 판단)
        o_bWaypointReached = (distance_to_wp < 10.0);
    }
}
